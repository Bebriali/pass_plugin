#include <cstdlib>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"
#include "llvm/IR/IRBuilder.h"
#include "defuse_plugin.hpp"
#include "defuse_dump_json.hpp"

using namespace llvm;

namespace llvm {
    PreservedAnalyses DefUsePluginPass::run(Module& module, ModuleAnalysisManager &analysis_manager) {
        auto rtd = setupRuntimeDumper(module);
        
        dumpDefuse(module, rtd.dumper);

        injectInstrumentation(module, rtd);

        return PreservedAnalyses::none();
    }
    
    DefUsePluginPass::RuntimeDumper DefUsePluginPass::setupRuntimeDumper(Module& module) {
        const char* dump_path = std::getenv("DEFUSE_IR_DUMP_PATH");
        JsonDumper dumper(dump_path);

        LLVMContext &ctx = module.getContext();

        Type *int64Ty = Type::getInt64Ty(ctx);
        Type *voidTy = Type::getVoidTy(ctx);

        return {
            module.getOrInsertFunction("initLogger", FunctionType::get(voidTy, {int64Ty}, false)), 
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

    void DefUsePluginPass::injectInstrumentation(Module& module, RuntimeDumper& rtd) {
        if (Function* main = module.getFunction("main")) {
            if (!main->isDeclaration()) {
                const char* env_path = std::getenv("DEFUSE_IR_DUMP_PATH");
                const char* path = (env_path && *env_path) ? env_path : "log/json/ir_dump.json";
                IRBuilder<> builder(&*main->getEntryBlock().getFirstInsertionPt());
                builder.CreateCall(rtd.initloggerfn, {builder.CreateGlobalString(path)});
            }
        }

        Type* int64Ty = Type::getInt64Ty(rtd.ctx);

        for (Function& function : module) {
            if (function.isDeclaration()) continue;
            for (BasicBlock& block : function) {
                for (Instruction& instruction : block) {
                    if (instruction.isTerminator()) continue;

                    if (auto* si = dyn_cast<StoreInst>(&instruction)) {
                        IRBuilder<> builder(&instruction);
                        Value* casted = getCastedValue(builder, si->getValueOperand(), int64Ty);
                        createInstrumentationCall(builder, rtd.logmemfn, instruction, casted, si->getPointerOperand());
                    } 
                    else if (auto* li = dyn_cast<LoadInst>(&instruction)) {
                        if (auto* next = instruction.getNextNode()) {
                            IRBuilder<> builder(next);
                            Value* casted = getCastedValue(builder, li, int64Ty);
                            createInstrumentationCall(builder, rtd.logmemfn, instruction, casted, li->getPointerOperand());
                        }
                    } 
                    else {
                        if (auto* next = instruction.getNextNode()) {
                            IRBuilder<> builder(next);
                            Value* casted = getCastedValue(builder, &instruction, int64Ty);
                            createInstrumentationCall(builder, rtd.loglongfn, instruction, casted);
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
