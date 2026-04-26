#ifndef LLVM_DEFUSE_DUMP_JSON_HPP
#define LLVM_DEFUSE_DUMP_JSON_HPP

#include "json.hpp"
#include <string>
#include <vector>
#include <fstream>

namespace llvm {
    class JsonDumper {
    private:
        nlohmann::json root;
        std::string filename;
        
        nlohmann::json* current_func = nullptr;
        nlohmann::json* current_block = nullptr;

    public:
        JsonDumper() = default;
        
        JsonDumper(std::string filename);

        ~JsonDumper();

        void addFunction(std::string name);
        void addBasicBlock(std::string id);
        
        void addInstruction(std::string id, std::string text, bool has_val, bool has_addr);

        void addEdge(std::string fromID, std::string toID, std::string type, std::string color);
        void addCallEdge(std::string caller, std::string callee, uint64_t id);
        
        void save();
    };
}

#endif