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

        int InstID = 0;
        std::map<Instruction*, int> InstToID;
        for (Function& F : M) {
            for (BasicBlock& BB : F) {
                for (Instruction& I : BB) {
                    InstToID[&I] = InstID++;
                }
            }
        }
    
        for (Function& f : M.functions()) {
            if (f.isDeclaration()) continue;
    
            for (BasicBlock& BB : f) {
                DotFile << "  subgraph \"cluster_" << &BB << "\" {\n";
                DotFile << "    label = \"Basic Block: " << &BB << "\";\n";
                DotFile << "    style = filled;\n";
                DotFile << "    color = lightgrey;\n";
    
                Instruction* PrevInst = nullptr;
    
                for (Instruction& I : BB) {
                    int currentID = InstToID[&I];
                    
                    DotFile << "// Instruction: " << I << "\n";
                    DotFile << "    \"inst_" << currentID << "\" [label=\"{ " << I 
                                << " | VALUE: <val_" << currentID << "> ? }\"];\n";
                    
                    if (PrevInst) {
                        // edge from prev to current instruction
                        DotFile << "    \"inst_" << InstToID[PrevInst] << "\" -> \"inst_" << currentID << "\" [style=bold];\n";
                    }
                    PrevInst = &I;
    
                    for (Use& U : I.operands()){
    
                        Value* v = U.get(); 
                        if (Instruction* DefInst = dyn_cast<Instruction>(U)) {
                            DotFile << "    \"inst_" << currentID << "\" -> \"inst_" << InstToID[DefInst] 
                                        << "\" [style=dashed, color=red, constraint=false];\n";
                        }
                    }
                }
                DotFile << "  } // subgraph" << &BB << "\n";
                
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
        }
    
        DotFile << "} // def-use\n"; // closing graph def-use
    
        for (auto const& [Inst, ID] : InstToID) {
            if (Inst->isTerminator()) continue;

            IRBuilder<> Builder(ctx);
            if (auto* Next = Inst->getNextNode()) {
                Builder.SetInsertPoint(Next);
            } else {
                Builder.SetInsertPoint(Inst->getParent());
            }

            Value* Valtolog = nullptr;
            if (auto* SI = dyn_cast<StoreInst>(Inst)) {
                Valtolog = SI->getValueOperand();
            } else if (!Inst->getType()->isVoidTy()) {
                Valtolog = Inst;
            }

            if (Valtolog) {
                Type* ValTy = Valtolog->getType();
                
                if (ValTy->isIntegerTy() && ValTy->getIntegerBitWidth() <= 32) {
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