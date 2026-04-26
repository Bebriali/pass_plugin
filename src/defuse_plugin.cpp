#include <cstdlib>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/IR/IRBuilder.h"
#include "defuse_plugin.hpp"
#include "defuse_dump_json.hpp"

using namespace llvm;

namespace llvm {
    PreservedAnalyses DefUsePluginPass::run(Module& module, ModuleAnalysisManager &analysis_manager) {
        auto rtd = setupRuntimeDumper(module);
        
        dumpDefuse(module, rtd.dumper);
        dumpCallGraph(module, rtd.dumper);

        injectInstrumentation(module, rtd);

        return PreservedAnalyses::none();
    }
    
    DefUsePluginPass::RuntimeDumper DefUsePluginPass::setupRuntimeDumper(Module& module) {
        const char* dump_path = std::getenv("DEFUSE_IR_DUMP_PATH");
        JsonDumper dumper(dump_path);

        LLVMContext &ctx = module.getContext();

        Type *int64Ty = Type::getInt64Ty(ctx);
        Type *voidTy = Type::getVoidTy(ctx);
        Type *Int8PtrTy = Type::getInt8PtrTy(ctx);

        return {
            module.getOrInsertFunction("initLogger", FunctionType::get(voidTy, {Int8PtrTy}, false)), 
            module.getOrInsertFunction("logLongVal", FunctionType::get(voidTy, {int64Ty, int64Ty}, false)),
            module.getOrInsertFunction("logMem", FunctionType::get(voidTy, {int64Ty, int64Ty, int64Ty}, false)),
            dumper,
            ctx
        }; 
    }
    
    void DefUsePluginPass::dumpDefuse(Module& module, JsonDumper& dumper) {
        auto toHexStr = [](void* ptr) -> std::string {
            std::stringstream ss;
            ss << "0x" << std::hex << reinterpret_cast<uintptr_t>(ptr);
            return ss.str();
        };
    
        auto instToString = [](Instruction& instruction) -> std::string {
            std::string s;
            raw_string_ostream os(s);
            instruction.print(os);
            return s;
        };

        for (Function& function : module) {
            if (function.isDeclaration()) continue;

            dumper.addFunction(function.getName().str());

            for (BasicBlock& block : function) {
                std::string bbId = toHexStr(&block);
                dumper.addBasicBlock(bbId);

                Instruction* prev_inst = nullptr;

                for (Instruction& instruction : block) {
                    std::string instId = toHexStr(&instruction);

                    bool hasV = !instruction.getType()->isVoidTy() || isa<StoreInst>(instruction);
                    bool hasA = isa<LoadInst>(instruction) || isa<StoreInst>(instruction);

                    dumper.addInstruction(instId, instToString(instruction), hasV, hasA);
                
                    if (prev_inst) {
                        // edge from prev to current instruction
                        dumper.addEdge(toHexStr(prev_inst), instId, "cfg", "black");
                    }
                    prev_inst = &instruction;

                    for (Use& u : instruction.operands()){
                        Value* val = u.get();
                        if (Instruction* def_inst = dyn_cast<Instruction>(val)) {
                            dumper.addEdge(instId, toHexStr(def_inst), "use", "red");
                        }
                    }
                }

                // connecting basic blocks
                Instruction *term = block.getTerminator();

                for (auto *successor: successors(term)) {
                    dumper.addEdge(toHexStr(term), toHexStr(&successor->front()), "cfg-link", "blue");
                }
            }
        }
        dumper.save();
    }

    Value* DefUsePluginPass::getCastedValue(IRBuilder<>& builder, Value* val, Type* destTy) {
        if (!val || val->getType()->isVoidTy()) return nullptr;

        // avoiding unnecessary casts
        Type* srcTy = val->getType();
        if (srcTy == destTy) return val;

        if (val->getType()->isIntegerTy()) {
            return builder.CreateIntCast(val, destTy, false);
        } 
        if (val->getType()->isPointerTy()) {
            return builder.CreatePtrToInt(val, destTy);
        }

        return nullptr;
    }

    void DefUsePluginPass::createInstrumentationCall(IRBuilder<>& builder, FunctionCallee func, 
                               Instruction& instruction, Value* casted_val, Value* addr) {
        if (!casted_val) return;

        uint64_t id = reinterpret_cast<uintptr_t>(&instruction);
        Value* idVal = builder.getInt64(id);

        SmallVector<Value*, 3> args;
        args.push_back(idVal);
        args.push_back(casted_val);
        if (addr) args.push_back(addr);

        builder.CreateCall(func, args);
    }

    void DefUsePluginPass::injectLoggerInit(Module& module, RuntimeDumper& rtd) {
        if (Function* main = module.getFunction("main")) {
            if (!main->isDeclaration()) {
                const char* env_path = std::getenv("DEFUSE_IR_DUMP_PATH");
                const char* path = (env_path && *env_path) ? env_path : "log/json/ir_dump.json";
                Type* Int8PtrTy = Type::getInt8PtrTy(rtd.ctx);
                IRBuilder<> builder(&*main->getEntryBlock().getFirstInsertionPt());
                Value* path_char = builder.CreateGlobalStringPtr(path);
                builder.CreateCall(rtd.initloggerfn, {path_char});
            }
        }
    }

    void DefUsePluginPass::chooseCall(Instruction& I, RuntimeDumper& rtd) {
        Type* int64Ty = Type::getInt64Ty(rtd.ctx);
        auto* next = I.getNextNode();
        uint64_t instId = reinterpret_cast<uintptr_t>(&I);

        if (auto* ci = dyn_cast<CallInst>(&I)) {
            IRBuilder<> builder(ci);
            for (unsigned i = 0; i < ci->arg_size(); ++i) {
                Value* arg = ci->getArgOperand(i);
                if (Value* castedArg = getCastedValue(builder, arg, int64Ty)) {
                    emitLog(builder, rtd.loglongfn, instId + i + 1, castedArg, nullptr);
                }
            }

            if (!ci->getType()->isVoidTy() && next) {
                IRBuilder<> nextBuilder(next);
                if (Value* castedRet = getCastedValue(nextBuilder, ci, int64Ty)) {
                    emitLog(nextBuilder, rtd.loglongfn, instId, castedRet, nullptr);
                }
            }
        }
        else if (auto* si = dyn_cast<StoreInst>(&I)) {
            IRBuilder<> builder(&I);
            Value* casted = getCastedValue(builder, si->getValueOperand(), int64Ty);
            Value* ptrAsInt = builder.CreatePtrToInt(si->getPointerOperand(), int64Ty);
            emitLog(builder, rtd.logmemfn, instId, casted, ptrAsInt);
        }
        else if (next) {
            IRBuilder<> builder(next);
            Value* casted = getCastedValue(builder, &I, int64Ty);
            if (auto* li = dyn_cast<LoadInst>(&I)) {
                Value* ptrAsInt = builder.CreatePtrToInt(li->getPointerOperand(), int64Ty);
                emitLog(builder, rtd.logmemfn, instId, casted, ptrAsInt);
            } else {
                emitLog(builder, rtd.loglongfn, instId, casted, nullptr);
            }
        }
    }

    void DefUsePluginPass::injectLoggerDefUse(Module& module, RuntimeDumper& rtd) {
        for (Function& function : module) {
            if (function.isDeclaration()) continue;
            injectFunctionArgs(function, rtd);

            for (BasicBlock& block : function) {
                for (Instruction& instruction : block) {

                    if (instruction.isTerminator()) continue;
                    
                    chooseCall(instruction, rtd);
                }
            }
        }
    }
    
    void DefUsePluginPass::injectInstrumentation(Module& module, RuntimeDumper& rtd) {
        injectLoggerInit(module, rtd);

        injectLoggerDefUse(module, rtd);
    }

    void DefUsePluginPass::emitLog(IRBuilder<>& builder, FunctionCallee func, 
                                   uint64_t id, Value* val, Value* addr) {
        if (!val) return;
        Value* idVal = builder.getInt64(id);
        SmallVector<Value*, 3> args = {idVal, val};
        if (addr) args.push_back(addr);
        builder.CreateCall(func, args);
    }

    void DefUsePluginPass::injectFunctionArgs(Function& function, RuntimeDumper& rtd) {
        if (function.isDeclaration() || function.getName().startswith("log") || function.getName() == "main") return;

        IRBuilder<> builder(&*function.getEntryBlock().getFirstInsertionPt());
        Type* int64Ty = Type::getInt64Ty(rtd.ctx);

        uint32_t argIdx = 0;
        for (Argument &arg : function.args()) {
            uint64_t argId = reinterpret_cast<uintptr_t>(&arg);
            
            Value* casted = getCastedValue(builder, &arg, int64Ty);
            if (casted) {
                emitLog(builder, rtd.loglongfn, argId, casted, nullptr);
            }
            argIdx++;
        }
    }

    void DefUsePluginPass::dumpCallGraph(Module& module, JsonDumper& dumper) {
        for (Function& function : module) {
            if (function.isDeclaration()) continue;

            for (BasicBlock& block : function) {
                for (Instruction& instruction : block) {
                    if (auto* ci = dyn_cast<CallInst>(&instruction)) {
                        Function* Callee = ci->getCalledFunction();
                        if (!Callee) continue;

                        std::string callerName = function.getName().str();
                        std::string calleeName = Callee->getName().str();
                        
                        dumper.addCallEdge(callerName, calleeName, reinterpret_cast<uintptr_t>(ci));
                    }
                }
            }
        }
    }

} // namespace llvm

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION,
        "DefUsePlugin",
        LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel) {
                    MPM.addPass(DefUsePluginPass());
                });

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
