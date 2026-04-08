#ifndef LLVM_DEFUSE_DUMP_JSON_HPP
#define LLVM_DEFUSE_DUMP_JSON_HPP

#include "json.hpp"
#include <string>
#include <vector>
#include <fstream>

using json = nlohmann::json;

namespace llvm {
    class JsonDumper {
    private:
        nlohmann::json root;
        std::string Filename;
        
        nlohmann::json* currentFunc = nullptr;
        nlohmann::json* currentBB = nullptr;

    public:
        JsonDumper() = default;
        JsonDumper(std::string Filename);

        void addFunction(std::string Name);
        void addBasicBlock(std::string ID, std::string Label);
        
        void addInstruction(std::string ID, std::string Text, bool hasVal, bool hasAddr);

        void addEdge(std::string FromID, std::string ToID, std::string Type, std::string Color);

        void save();
    };
}

#endif