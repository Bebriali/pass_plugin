#ifndef LLVM_DEFUSE_DUMP_DOT_HPP
#define LLVM_DEFUSE_DUMP_DOT_HPP

#include <system_error>
#include <cstdint>

#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instruction.h"

namespace llvm {
    class GraphDumper {
    private:
        std::optional<llvm::raw_fd_ostream> Out;

    public:
        GraphDumper() = default;
        GraphDumper(llvm::StringRef Filename) {
            std::error_code EC;

            Out.emplace(Filename, EC);

            if (EC) {
                llvm::errs() << "Failure opening file: " << EC.message() << "\n";
            }
        }

        ~GraphDumper() = default;

        void close();

        void dumpGraphDef();

        void openSubgraphF(llvm::StringRef Name);

        void openSubgraphBB(void* Addr);
        void closeSubgraph();

        void closeGraph();

        void dumpInstruction(llvm::Instruction& I, void* ID);

        void dumpVal(void* ID);

        void dumpAddr(void* ID);

        void dumpClose();

        void addEdge(void* FromID, void* ToID, llvm::StringRef Style, llvm::StringRef Color);
    };
}

#endif