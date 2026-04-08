#include <iostream>
#include <unordered_map>

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"
#include "llvm/IR/IRBuilder.h"
#include "defuse_plugin.hpp"
#include "defuse_dump_json.hpp"

using namespace llvm;

namespace llvm {
    PreservedAnalyses DefUsePluginPass::run(Module& M, ModuleAnalysisManager &AM) {
        auto setup = prepareContext(M);
        
        dumpDefuse(M, setup.dumper);

        instrumentation(M, setup);

        return PreservedAnalyses::none();
    }
    
    DefUsePluginPass::RuntimeInfo DefUsePluginPass::setupRuntime(Module& M, LLVMContext& ctx) {
        Type *I64Ty = Type::getInt64Ty(ctx);
        Type *PtrTy = PointerType::get(ctx, 0);
        Type *VoidTy = Type::getVoidTy(ctx);

        return {
            M.getOrInsertFunction("log_long_val", FunctionType::get(VoidTy, {I64Ty, I64Ty}, false)),
            M.getOrInsertFunction("log_mem", FunctionType::get(VoidTy, {I64Ty, I64Ty, PtrTy}, false))
        };
    }

    DefUsePluginPass::SetupContext DefUsePluginPass::prepareContext(Module& M) {
        JsonDumper dumper("log/json/ir_dump.json");

        LLVMContext &ctx = M.getContext();
        RuntimeInfo rt = setupRuntime(M, ctx);

        return {std::move(dumper), ctx, rt};
    }
    
    void DefUsePluginPass::dumpDefuse(Module& M, JsonDumper& dumper) {
        auto toHexStr = [](void* P) -> std::string {
            std::stringstream ss;
            ss << "0x" << std::hex << reinterpret_cast<uintptr_t>(P);
            return ss.str();
        };
    
        auto instToString = [](Instruction& I) -> std::string {
            std::string s;
            raw_string_ostream os(s);
            I.print(os);
            return s;
        };

        for (Function& F : M) {
            if (F.isDeclaration()) continue;

            dumper.addFunction(F.getName().str());

            for (BasicBlock& BB : F) {
                std::string bbId = toHexStr(&BB);
                dumper.addBasicBlock(bbId, "BB: " + bbId);

                Instruction* PrevInst = nullptr;

                for (Instruction& I : BB) {
                    std::string instId = toHexStr(&I);

                    bool hasV = !I.getType()->isVoidTy() || isa<StoreInst>(I);
                    bool hasA = isa<LoadInst>(I) || isa<StoreInst>(I);

                    dumper.addInstruction(instId, instToString(I), hasV, hasA);
                
                    if (PrevInst) {
                        // edge from prev to current instruction
                        dumper.addEdge(toHexStr(PrevInst), instId, "cfg", "black");
                    }
                    PrevInst = &I;

                    for (Use& U : I.operands()){

                        Value* v = U.get();
                        if (Instruction* DefInst = dyn_cast<Instruction>(v)) {
                            dumper.addEdge(instId, toHexStr(DefInst), "use", "red");
                        }
                    }
                }

                // connecting basic blocks
                Instruction *Term = BB.getTerminator();

                for (unsigned i = 0; i < Term->getNumSuccessors(); ++i) {
                    BasicBlock *Successor = Term->getSuccessor(i);
                    dumper.addEdge(toHexStr(Term), toHexStr(&Successor->front()), "cfg-link", "blue");
                }
            }
        }
        dumper.save();
    }

    void DefUsePluginPass::instrumentation(Module& M, SetupContext& S) {
        for (Function& F : M) {
            if (F.isDeclaration()) continue;
            for (BasicBlock& BB : F) {
                for (Instruction& I : BB) {

                    if (I.isTerminator()) continue;
                    
                    IRBuilder<> Builder(S.ctx);
                    uint64_t ID = reinterpret_cast<uintptr_t>(&I);
                    Type* I64Ty = Type::getInt64Ty(S.ctx);
        
                    if (auto* SI = dyn_cast<StoreInst>(&I)) {
                        Builder.SetInsertPoint(&I);
        
                        Value* Val = SI->getValueOperand();
                        Value* Addr = SI->getPointerOperand();
                        
                        Value* CastedVal = Val->getType()->isPointerTy() ? 
                            Builder.CreatePtrToInt(Val, I64Ty) : 
                            Builder.CreateIntCast(Val, I64Ty, false);

                        Builder.CreateCall(S.rt.logmemfn, {Builder.getInt64(ID), CastedVal, Addr});
                    }
                    else if (auto* LI = dyn_cast<LoadInst>(&I)) {
                        auto* Next = I.getNextNode();
                        if (Next) Builder.SetInsertPoint(Next);

                        Value* Addr = LI->getPointerOperand();
                        Value* CastedVal = nullptr;

                        if (I.getType()->isPointerTy()) {
                            CastedVal = Builder.CreatePtrToInt(&I, I64Ty);
                        } else {
                            CastedVal = Builder.CreateIntCast(&I, I64Ty, false);
                        }

                        if (CastedVal) {
                            Builder.CreateCall(S.rt.logmemfn, {Builder.getInt64(ID), CastedVal, Addr});
                        }
                    }
                    else if (!I.getType()->isVoidTy()) {
                        auto* Next = I.getNextNode();
                        if (Next) Builder.SetInsertPoint(Next);

                        Value* CastedVal = nullptr;
                        if (I.getType()->isIntegerTy()) {
                            CastedVal = Builder.CreateIntCast(&I, I64Ty, false);
                        } else if (I.getType()->isPointerTy()) {
                            CastedVal = Builder.CreatePtrToInt(&I, I64Ty);
                        }

                        if (CastedVal) {
                            Builder.CreateCall(S.rt.loglongfn, {Builder.getInt64(ID), CastedVal});
                        }
                    }
                }
            }
        }
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
