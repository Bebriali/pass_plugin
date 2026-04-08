#ifndef LLVM_DEFUSE_PASS_PLUGIN_HPP
#define LLVM_DEFUSE_PASS_PLUGIN_HPP

#include "defuse_dump_json.hpp"

namespace llvm {
    class DefUsePluginPass : public PassInfoMixin<DefUsePluginPass> {
        struct RuntimeInfo {
            FunctionCallee loglongfn; // only value
            FunctionCallee logmemfn;  // value + addr
        };
        struct SetupContext {
            JsonDumper dumper;
            LLVMContext &ctx;
            RuntimeInfo rt;
        };
    public:
        /**
         * analyses module
         * dumping def-use graph to *.dot file
         * does not affect IR
         */
        PreservedAnalyses run(Module& M, ModuleAnalysisManager& AM);
    private:
        RuntimeInfo setupRuntime(Module& M, LLVMContext& ctx);
        SetupContext prepareContext(Module& M);
        void dumpDefuse(Module& M, JsonDumper& dumper);
        void instrumentation(Module& M, SetupContext& S);
    };
} // namespace llvm

#endif // LLVM_DEFUSE_PASS_PLUGIN_HPP
