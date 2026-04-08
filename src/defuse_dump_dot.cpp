#include <unordered_map>

#include "defuse_dump_dot.hpp"

using namespace llvm;

void llvm::GraphDumper::close() {
    if (Out) {
        Out->close();
        Out.reset();
    }
}

void llvm::GraphDumper::dumpGraphDef() {
    if (Out) {
        *Out << "digraph \"def-use\" {\n"
            << "  compound=true;\n"
            << "  node [shape=record, fontname=\"Courier\"];\n";
    }
}

void llvm::GraphDumper::openSubgraphF(llvm::StringRef Name) {
    if (Out)
    {
        *Out << "  subgraph \"cluster_" << Name.str() << "\" {\n"
            << "    label = \"Function: " << Name.str() << "\";\n"
            << "    fontsize = 30; fontname = \"Arial Bold\";\n"
            << "    style = filled; color = black; fillcolor = white; penwidth = 3.0;\n";
    }
}

void llvm::GraphDumper::openSubgraphBB(void* Addr) {
    if (Out) {
        *Out << "  subgraph \"cluster_" << Addr << "\" {\n"
            << "    label = \"Basic Block: " << Addr << "\";\n"
            << "    style = filled; color = black; penwidth = 2.0; fillcolor = pink;\n";
    }
}

void llvm::GraphDumper::closeSubgraph() { 
    if (Out) {
        *Out << "  }\n";
    }
}

void llvm::GraphDumper::closeGraph() { 
    if (Out) {
        *Out << "}\n"; 
    }
}

void llvm::GraphDumper::dumpInstruction(llvm::Instruction& I, void* ID) {
    if (Out) {
        *Out << "    \"inst_" << ID << "\" [label=\"{ " << I;
    }   
}

void llvm::GraphDumper::dumpVal(void* ID) {
    if (Out) {
        *Out << " | VALUE: <val_" << ID << "> ? ";
    }
}

void llvm::GraphDumper::dumpAddr(void* ID) {
    if (Out) {
        *Out << " | ADDR: <addr_" << ID << "> ? ";
    }
}

void llvm::GraphDumper::dumpClose() {
    if (Out) {
        *Out << " }\"];\n";
    }
}

void llvm::GraphDumper::addEdge(void* FromID, void* ToID, llvm::StringRef Style, llvm::StringRef Color) {
    if (Out) {
        *Out << "    \"inst_" << FromID << "\" -> \"inst_" << ToID
        << "\" [style=" << Style << ", color=" << Color << "];\n";
    }
}