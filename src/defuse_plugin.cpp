#include <iostream>
#include <map>

#include "PassBuilder.h"
#include "PassPlugin.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "defuse_plugin.hpp"

using namespace llvm;

namespace llvm {

    PreservedAnalyses DefUsePluginPass::run(Module& M, ModuleAnalysisManager &AM) {
        dumper = GraphDumper("log/dot/graph.dot");

        dumper.dumpGraphDef();

        LLVMContext &ctx = M.getContext();
        FunctionCallee loglongfn = M.getOrInsertFunction("log_long_val",
            FunctionType::get(Type::getVoidTy(ctx),
            {Type::getInt32Ty(ctx), Type::getInt64Ty(ctx)},
            false));

        FunctionCallee logfn = M.getOrInsertFunction("log_val",
            FunctionType::get(Type::getVoidTy(ctx),
            {Type::getInt32Ty(ctx), Type::getInt32Ty(ctx)},
            false));

        FunctionCallee logaddrfn = M.getOrInsertFunction("log_addr",
            FunctionType::get(Type::getVoidTy(ctx),
            {Type::getInt32Ty(ctx), PointerType::get(ctx, 0)},
            false));

        // P.S.[flops]: Better completely remove it, you can use just instruction address as instruction ID
        std::map<Instruction*, int> InstToID;
        buildInstMap(M, InstToID);

        for (Function& F : M) {
            if (F.isDeclaration()) continue;

            dumper.openSubgraphF(F.getName());

            for (BasicBlock& BB : F) {
                dumper.openSubgraphBB(&BB);

                Instruction* PrevInst = nullptr;

                for (Instruction& I : BB) {
                    int currentID = InstToID[&I];

                    dumper.dumpInstruction(I, currentID);
                    if (!I.getType()->isVoidTy() || isa<StoreInst>(I)) {
                        dumper.dumpVal(currentID);
                    }
                    if (isa<LoadInst>(I) || isa<StoreInst>(I)) {
                        dumper.dumpAddr(currentID);
                    }
                    dumper.dumpClose();

                    if (PrevInst) {
                        // edge from prev to current instruction
                        dumper.addEdge(InstToID[PrevInst], currentID, "bold");
                    }
                    PrevInst = &I;

                    for (Use& U : I.operands()){

                        Value* v = U.get();
                        if (Instruction* DefInst = dyn_cast<Instruction>(v)) {
                            dumper.addEdge(currentID, InstToID[DefInst], "dashed", "red");
                        }
                    }
                }
                dumper.closeSubgraph();

                // connecting basic blocks
                Instruction *Term = BB.getTerminator();
                int TermID = InstToID[Term];

                for (unsigned i = 0; i < Term->getNumSuccessors(); ++i) {
                    BasicBlock *Successor = Term->getSuccessor(i);
                    Instruction *FirstInst = &Successor->front();

                    int TermID = InstToID[Term];
                    int DestID = InstToID[FirstInst];
                    dumper.addEdge(TermID, DestID, "bold", "blue");
                }
            }
            dumper.closeSubgraph();
        }
        dumper.closeGraph();
        dumper.close();

        for (auto const& [Inst, ID] : InstToID) {
            if (Inst->isTerminator()) continue;

            IRBuilder<> Builder(ctx);

            /** [Explanation]:
             * store insert point should be before the instruction execution cause
             * it does not produce any value
             * we know both ptr and val before storing
             * after the store ptr or val can be unreachable
             * (if their timeline is over)
             */
            if (isa<StoreInst>(Inst)) {
                Builder.SetInsertPoint(Inst);
            }
            else {
                auto* Next = Inst->getNextNode();
                if (Next) Builder.SetInsertPoint(Next);
                else Builder.SetInsertPoint(Inst->getParent());
            }

            Value* Valtolog = nullptr;
            Value* Addrtolog = nullptr;

            // specific for load/store as for the only ones working with addr's
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
                // using specific flag for addr values, cause default values have
                // the same instruction id
                Builder.CreateCall(logaddrfn, {Builder.getInt32(ID | ADDR_FLAG), Addrtolog});
            }

            if (Valtolog) {
                Type* ValTy = Valtolog->getType();
                Type* I32Ty = Type::getInt32Ty(ctx);
                Type* I64Ty = Type::getInt64Ty(ctx);

                if (ValTy->isIntegerTy()) {
                    unsigned BitWidth = ValTy->getIntegerBitWidth();

                    if (BitWidth <= 32) {
                        Value* ExtVal = Builder.CreateIntCast(Valtolog, I32Ty, false);
                        Builder.CreateCall(logfn, {Builder.getInt32(ID), ExtVal});
                    }
                    else {
                        Value* ExtVal = Builder.CreateIntCast(Valtolog, I64Ty, false);
                        Builder.CreateCall(loglongfn, {Builder.getInt32(ID), ExtVal});
                    }
                }
                else if (ValTy->isPointerTy()) {
                    Value* CastedPtr = Builder.CreatePtrToInt(Valtolog, I64Ty);
                    Builder.CreateCall(loglongfn, {Builder.getInt32(ID), CastedPtr});
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
