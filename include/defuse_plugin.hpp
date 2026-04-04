#ifndef LLVM_DEFUSE_PASS_PLUGIN_HPP
#define LLVM_DEFUSE_PASS_PLUGIN_HPP

#include <system_error>
#include <cstdint>

#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instruction.h"

namespace llvm {
    class GraphDumper {
    private:
        llvm::raw_fd_ostream *Out = nullptr;
        std::error_code EC;

    public:
        GraphDumper() = default;
        GraphDumper(llvm::StringRef Filename) {
            Out = new llvm::raw_fd_ostream(Filename, EC);
        }

        void close() {
            if (Out) {
                Out->close();
                delete Out;
                Out = nullptr;
            }
        }

        void dumpGraphDef() {
            if (Out) {
                *Out << "digraph \"def-use\" {\n"
                    << "  compound=true;\n"
                    << "  node [shape=record, fontname=\"Courier\"];\n";
            }
        }

        void openSubgraphF(llvm::StringRef Name) {
            *Out << "  subgraph \"cluster_" << Name.str() << "\" {\n"
                << "    label = \"Function: " << Name.str() << "\";\n"
                << "    fontsize = 30; fontname = \"Arial Bold\";\n"
                << "    style = filled; color = black; fillcolor = white; penwidth = 3.0;\n";
        }

        void openSubgraphBB(void* Addr) {
            *Out << "  subgraph \"cluster_" << Addr << "\" {\n"
                << "    label = \"Basic Block: " << Addr << "\";\n"
                << "    style = filled; color = black; penwidth = 2.0; fillcolor = pink;\n";
        }

        void closeSubgraph() { *Out << "  }\n"; }

        void closeGraph() { *Out << "}\n"; }

        void dumpInstruction(llvm::Instruction &I, int ID) {
            *Out << "    \"inst_" << ID << "\" [label=\"{ " << I;
        }

        void dumpVal(int ID) {
            if (Out) {
                *Out << " | VALUE: <val_" << ID << "> ? ";
            }
        }

        void dumpAddr(int ID) {
            if (Out) {
                *Out << " | ADDR: <addr_" << ID << "> ? ";
            }
        }

        void dumpClose() {
            *Out << " }\"];\n";
        }

        void addEdge(int FromID, int ToID, llvm::StringRef Style = "bold", llvm::StringRef Color = "black") {
            *Out << "    \"inst_" << FromID << "\" -> \"inst_" << ToID
                << "\" [style=" << Style << ", color=" << Color << "];\n";
        }
    };

    class DefUsePluginPass : public PassInfoMixin<DefUsePluginPass> {
        GraphDumper dumper;
        static const uint32_t ADDR_FLAG = 0x80000000;
    public:
        /**
         * analyses module
         * dumping def-use graph to *.dot file
         * does not affect IR
         */
        PreservedAnalyses run(Module& M, ModuleAnalysisManager& AM);
    private:
        void buildInstMap(Module &M, std::map<Instruction*, int> &InstToID) {
            int InstID = 0;
            for (Function &F : M) {
                if (F.isDeclaration()) continue;
                for (BasicBlock &BB : F) {
                    for (Instruction &I : BB) {
                        InstToID[&I] = InstID++;
                    }
                }
            }
        }
    };
} // namespace llvm

#endif // LLVM_DEFUSE_PASS_PLUGIN_HPP
