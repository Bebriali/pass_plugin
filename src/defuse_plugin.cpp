#include <iostream>
#include <map>

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "defuse_plugin.hpp"

using namespace llvm;

namespace llvm {

    PreservedAnalyses DefUsePluginPass::run(Module& M, ModuleAnalysisManager &AM) {
        std::error_code EC;
        raw_fd_ostream DotFile("log/dot/graph.dot", EC, sys::fs::OF_Text);
        if (EC)
            return PreservedAnalyses::all();
    
        // FIXME[flops]: Move ALL Dot dump functional into separate funcs/class or
        // You can dump info about def use graph in machine-readable format like JSON/CSV
        // And generate graph in overlay.py via pygraphviz/networkx e.t.c.

        DotFile << "digraph \"def-use\" {\n"; // the declaration of graph in *.dot
        DotFile << "  compound=true;\n";
        DotFile << "  node [shape=box, fontname=\"Courier\"];\n";
    
        LLVMContext &ctx = M.getContext();
        FunctionCallee loglongfn = M.getOrInsertFunction("__log_long_val", 
            FunctionType::get(Type::getVoidTy(ctx), 
            {Type::getInt32Ty(ctx), Type::getInt64Ty(ctx)}, 
            false));

        FunctionCallee logfn = M.getOrInsertFunction("__log_val", 
            FunctionType::get(Type::getVoidTy(ctx), 
            {Type::getInt32Ty(ctx), Type::getInt32Ty(ctx)}, 
            false));

        FunctionCallee logaddrfn = M.getOrInsertFunction("__log_addr", 
            FunctionType::get(Type::getVoidTy(ctx), 
            {Type::getInt32Ty(ctx), PointerType::get(ctx, 0)},
            false));

        // FIXME[flops]: Move it to the separate function
        // P.S.[flops]: Better completely remove it, you can use just instruction address as instruction ID
        int InstID = 0;
        std::map<Instruction*, int> InstToID;
        for (Function& F : M) {
            for (BasicBlock& BB : F) {
                for (Instruction& I : BB) {
                    InstToID[&I] = InstID++;
                }
            }
        }
    
        // FIXME[flops]: inconsistent naming: use only one specific case
        // TODO[flops]: You may use just `for (Function& f : M)`
        for (Function& f : M.functions()) {
            if (f.isDeclaration()) continue;
    
            DotFile << "  subgraph \"cluster_" << f.getName().str() << "\" {\n";

            DotFile << "    label = \"Function: " << f.getName().str() << "\";\n";
            DotFile << "    fontsize = 30;\n";
            DotFile << "    fontname = \"Arial Bold\";\n";

            DotFile << "    style = filled;\n";
            DotFile << "    color = black;\n";
            DotFile << "    fillcolor = white;\n";
            DotFile << "    penwidth = 3.0;\n";

            for (BasicBlock& BB : f) {
                DotFile << "  subgraph \"cluster_" << &BB << "\" {\n";
                DotFile << "    label = \"Basic Block: " << &BB << "\";\n";
                DotFile << "    style = filled;\n";
                DotFile << "    color = black;\n";
                DotFile << "    penwidth = 2.0;\n";
                DotFile << "    fillcolor = pink;\n";
    
                Instruction* PrevInst = nullptr;

                // FIXME[flops]: Insts vector is useless, you can just iterate trough BB there --\ 
                std::vector<Instruction*> Insts; //                                               |
                for (Instruction& I : BB) { //                                                    |
                    Insts.push_back(&I); //                                                       |
                } //                                                                              |
                //                                                                                |
                for (Instruction* I : Insts) { // <----------------------------------------------/
                    int currentID = InstToID[I];
                    
                    DotFile << "// Instruction: " << *I << "\n";
                    DotFile << "    \"inst_" << currentID << "\" [label=\"{ " << *I;
                    if (!I->getType()->isVoidTy() || isa<StoreInst>(I)) {
                        DotFile << " | VALUE: <val_" << currentID << "> ? ";
                    }
                    if (isa<LoadInst>(I) || isa<StoreInst>(I)) {
                        DotFile << " | ADDR: <addr_" << currentID << "> ? ";
                    }
                    DotFile << " }\"];\n";
                    
                    if (PrevInst) {
                        // edge from prev to current instruction
                        DotFile << "    \"inst_" << InstToID[PrevInst] << "\" -> \"inst_" << currentID << "\" [style=bold];\n";
                    }
                    PrevInst = I;
    
                    for (Use& U : I->operands()){
    
                        Value* v = U.get(); 
                        if (Instruction* DefInst = dyn_cast<Instruction>(U)) {
                            DotFile << "    \"inst_" << currentID << "\" -> \"inst_" << InstToID[DefInst] 
                                        << "\" [style=dashed, color=red, constraint=false];\n";
                        }
                    }
                }
                DotFile << "  } // basic block " << &BB << "subgraph" << "\n";
                
                // connecting basic blocks
                Instruction *Term = BB.getTerminator();
                int TermID = InstToID[Term];
                
                for (unsigned i = 0; i < Term->getNumSuccessors(); ++i) {
                    BasicBlock *Successor = Term->getSuccessor(i);
                    Instruction *FirstInst = &Successor->front();
                    
                    int TermID = InstToID[Term];
                    int DestID = InstToID[FirstInst];
                    DotFile << "  \"inst_" << TermID << "\" -> \"inst_" << DestID 
                            << "\" [penwidth=2, color=blue, weight=0];\n";
                }
            }

            DotFile << "  } // functional subgraph" << f.getName() << "\n";
        }
    
        DotFile << "} // def-use\n"; // closing graph def-use

        // FIXME[flops]: InstrumentationList is completely useless there
        std::vector<Instruction*> InstrumentationList;
        for (auto const& [Inst, ID] : InstToID) {
            InstrumentationList.push_back(Inst);
        }
    
        // FIXME[flops]: `for (auto const& [Inst, ID]: InstToID)`
        for (Instruction* Inst : InstrumentationList) {
            if (Inst->isTerminator()) continue;

            int ID = InstToID[Inst]; // FIXME[flops]: You already have instr ID

            IRBuilder<> Builder(ctx);

            if (isa<StoreInst>(Inst)) { // [flops]: Better add comment why StoreInst has different handle
                Builder.SetInsertPoint(Inst);
            } 
            else {
                auto* Next = Inst->getNextNode();
                if (Next) Builder.SetInsertPoint(Next);
                else Builder.SetInsertPoint(Inst->getParent());
            }

            Value* Valtolog = nullptr;
            Value* Addrtolog = nullptr;

            // specific for load/store
            if (auto* LI = dyn_cast<LoadInst>(Inst)) {
                Valtolog = Inst;
                Addrtolog = LI->getPointerOperand();
            } 
            else if (auto* SI = dyn_cast<StoreInst>(Inst)) {
                Valtolog = SI->getValueOperand();
                Addrtolog = SI->getPointerOperand();
                Builder.SetInsertPoint(Inst);
            } 
            else if (!Inst->getType()->isVoidTy()) {
                Valtolog = Inst;
            }

            if (Addrtolog) {
                Builder.CreateCall(logaddrfn, {Builder.getInt32(ID + 10000), Addrtolog}); // BUG //FIXME[flops]: Magic constant, why 10000?
            }

            if (Valtolog) {
                Type* ValTy = Valtolog->getType();
                
                if (ValTy->isIntegerTy() && ValTy->getIntegerBitWidth() <= 32) { //FIXME[flops]: Magic hardcoded constant. You can make it more flexible via cast using Builder
                    Value* ExtVal = Builder.CreateZExt(Valtolog, Type::getInt32Ty(ctx));
                    Builder.CreateCall(logfn, {Builder.getInt32(ID), ExtVal});
                }
                else if (ValTy->isIntegerTy(64) || ValTy->isPointerTy()) {
                    Value* CastedVal = Valtolog;
                    if (ValTy->isPointerTy()) {
                        CastedVal = Builder.CreatePtrToInt(Valtolog, Type::getInt64Ty(ctx));
                    }

                    Builder.CreateCall(loglongfn, {Builder.getInt32(ID), CastedVal});
                }
            }
        }

        return PreservedAnalyses::all();
    }
} // namespace llvm

/**
 * plugin registration
 * to not build all llvm on my desk
 */
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, 
        "DefUsePlugin", 
        LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "def-use-plugin") {
                        MPM.addPass(DefUsePluginPass());
                        return true;
                    }
                    return false;
                });
        }
    };
}