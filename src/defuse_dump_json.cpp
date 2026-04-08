#include "defuse_dump_json.hpp"

namespace llvm {

JsonDumper::JsonDumper(std::string Filename) : Filename(std::move(Filename)) {
    root["functions"] = json::array();
    root["edges"] = json::array();
}

void JsonDumper::addFunction(std::string Name) {
    root["functions"].push_back({
        {"name", std::move(Name)},
        {"blocks", json::array()}
    });
    currentFunc = &root["functions"].back();
}

void JsonDumper::addBasicBlock(std::string ID, std::string Label) {
    if (!currentFunc) return;

    (*currentFunc)["blocks"].push_back({
        {"id", std::move(ID)},
        {"label", std::move(Label)},
        {"instructions", json::array()}
    });
    currentBB = &((*currentFunc)["blocks"].back());
}

void JsonDumper::addInstruction(std::string ID, std::string Text, bool hasVal, bool hasAddr) {
    if (!currentBB) return;

    (*currentBB)["instructions"].push_back({
        {"id", std::move(ID)},
        {"text", std::move(Text)},
        {"has_val", hasVal},
        {"has_addr", hasAddr}
    });
}

void JsonDumper::addEdge(std::string FromID, std::string ToID, std::string Type, std::string Color) {
    root["edges"].push_back({
        {"from", std::move(FromID)},
        {"to", std::move(ToID)},
        {"type", std::move(Type)},
        {"color", std::move(Color)}
    });
}

void JsonDumper::save() {
    std::ofstream out(Filename);
    if (out.is_open()) {
        out << root.dump(4) << std::endl;
    }
}

} // namespace llvm