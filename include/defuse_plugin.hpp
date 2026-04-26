#ifndef LLVM_DEFUSE_PASS_PLUGIN_HPP
#define LLVM_DEFUSE_PASS_PLUGIN_HPP

#include "defuse_dump_json.hpp"

namespace llvm {
    class DefUsePluginPass : public PassInfoMixin<DefUsePluginPass> {
        struct RuntimeDumper {
            FunctionCallee initloggerfn; // void init_logger(const char* filename)
            FunctionCallee loglongfn; // only value
            FunctionCallee logmemfn;  // value + addr
            JsonDumper dumper;
            LLVMContext &ctx;
        };
    public:
        /**
         * analyses module
         * inserts instrumentation for logging def-use info at runtime
         * does not affect IR
         */
        PreservedAnalyses run(Module& module, ModuleAnalysisManager& ananlysis_manager);
    private:
        RuntimeDumper setupRuntimeDumper(Module& module);
        void dumpDefuse(Module& module, JsonDumper& dumper);
        void dumpCallGraph(Module& module, JsonDumper& dumper);

        void injectLoggerInit(Module& M, RuntimeDumper& rtd);
        void injectLoggerDefUse(Module& module, RuntimeDumper& rtd);

        void createInstrumentationCall(IRBuilder<>& builder, FunctionCallee func, 
            Instruction& instruction, Value* casted_val, Value* addr = nullptr);
        void chooseCall(Instruction& instruction, RuntimeDumper& rtd);
        void emitLog(IRBuilder<>& builder, FunctionCallee func, 
                                   uint64_t id, Value* val, Value* addr);
        void injectInstrumentation(Module& module, RuntimeDumper& rtd);
        void injectFunctionArgs(Function& function, RuntimeDumper& rtd);


        Value* getCastedValue(IRBuilder<>& builder, Value* val, Type* destTy);
    };
} // namespace llvm

#endif // LLVM_DEFUSE_PASS_PLUGIN_HPP
