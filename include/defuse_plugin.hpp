    #ifndef LLVM_DEFUSE_PASS_PLUGIN_HPP
    #define LLVM_DEFUSE_PASS_PLUGIN_HPP

    #include "llvm/IR/PassManager.h"

    namespace llvm {
        class DefUsePluginPass : public PassInfoMixin<DefUsePluginPass> {
        public:
            /**
             * analyses module
             * dumping def-use graph to *.dot file
             * does not affect IR
             */
            PreservedAnalyses run(Module& M, ModuleAnalysisManager& AM);
        };
    } // namespace llvm

    #endif // LLVM_DEFUSE_PASS_PLUGIN_HPP
