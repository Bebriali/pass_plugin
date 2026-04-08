#ifndef LLVM_DEFUSE_PASS_PLUGIN_HPP
#define LLVM_DEFUSE_PASS_PLUGIN_HPP

#include "defuse_dump_json.hpp"

namespace llvm {
    struct SetupContext {
        JsonDumper dumper;
        LLVMContext &ctx;
        FunctionCallee loglongfn;
        FunctionCallee logfn;
        FunctionCallee logaddrfn;
        FunctionCallee logmemfn;
    };

    struct RuntimeInfo {
        FunctionCallee loglongfn;
        FunctionCallee logmemfn;
    };

    class DefUsePluginPass : public PassInfoMixin<DefUsePluginPass> {
        static const uint32_t ADDR_FLAG = 0x80000000;
    public:

        /**
         * analyses module
         * dumping def-use graph to *.dot file
         * does not affect IR
         */
        PreservedAnalyses run(Module& M, ModuleAnalysisManager& AM);
    private:
        RuntimeInfo setupRuntime(Module& M, LLVMContext& ctx);
        SetupContext initSetup(Module& M);
        void dumpDefuse(Module& M, JsonDumper& dumper);
        void instrumentation(Module& M, SetupContext& S);

    };
} // namespace llvm

#endif // LLVM_DEFUSE_PASS_PLUGIN_HPP
