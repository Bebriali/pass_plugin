#include <assert.h>

#include "defuse_dump_json.hpp"

namespace llvm {

JsonDumper::JsonDumper(std::string filename) : filename(std::move(filename)) {
    root["functions"] = nlohmann::json::array();
    root["edges"] = nlohmann::json::array();
    root["values"] = nlohmann::json::array();
    root["call_edges"] = nlohmann::json::array();
}

JsonDumper::~JsonDumper() {
    if (!filename.empty()) {
        save();
    }
}

void JsonDumper::addFunction(std::string name) {
    root["functions"].push_back({
        {"name", std::move(name)},
        {"blocks", nlohmann::json::array()}
    });
    current_func = &root["functions"].back();
}

void JsonDumper::addBasicBlock(std::string id) {
    assert(current_func);

    (*current_func)["blocks"].push_back({
        {"id", std::move(id)},
        {"instructions", nlohmann::json::array()}
    });
    current_block = &((*current_func)["blocks"].back());
}

void JsonDumper::addInstruction(std::string id, std::string text, bool has_val, bool has_addr) {
    assert(current_block);

    (*current_block)["instructions"].push_back({
        {"id", std::move(id)},
        {"text", std::move(text)},
        {"has_val", has_val},
        {"has_addr", has_addr}
    });
}

void JsonDumper::addEdge(std::string fromID, std::string toID, std::string type, std::string color) {
    root["edges"].push_back({
        {"from", std::move(fromID)},
        {"to", std::move(toID)},
        {"type", std::move(type)},
        {"color", std::move(color)}
    });
}

void JsonDumper::addCallEdge(std::string caller, std::string callee, uint64_t id) {
    std::stringstream ss;
    ss << "0x" << std::hex << id;
    std::string idStr = ss.str();

    root["call_edges"].push_back({
        {"caller", std::move(caller)},
        {"callee", std::move(callee)},
        {"id", idStr}
    });
}

void JsonDumper::save() {
    std::ofstream out(filename);
    if (out.is_open()) {
        out << root.dump(4) << std::endl;
    }
}

} // namespace llvm