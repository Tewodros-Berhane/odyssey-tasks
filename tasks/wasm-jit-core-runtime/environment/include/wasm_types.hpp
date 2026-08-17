#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <optional>
#include <span>

namespace wasm {

enum class ValType : uint8_t {
    I32 = 0x7F,
    I64 = 0x7E,
    F32 = 0x7D,
    F64 = 0x7C,
    VOID = 0x40
};

enum class SectionId : uint8_t {
    CUSTOM = 0,
    TYPE = 1,
    IMPORT = 2,
    FUNCTION = 3,
    TABLE = 4,
    MEMORY = 5,
    GLOBAL = 6,
    EXPORT = 7,
    START = 8,
    ELEMENT = 9,
    CODE = 10,
    DATA = 11
};

struct FuncType {
    std::vector<ValType> params;
    std::vector<ValType> returns;
};

struct ExportEntry {
    std::string name;
    uint8_t kind;
    uint32_t index;
};

struct FunctionBody {
    std::vector<std::pair<uint32_t, ValType>> locals;
    std::vector<uint8_t> code;
};

struct Module {
    std::vector<FuncType> types;
    std::vector<uint32_t> functions; // Type indices
    std::vector<ExportEntry> exports;
    std::vector<FunctionBody> bodies;
    uint32_t initial_memory_pages{1};
    uint32_t max_memory_pages{1};
};

} // namespace wasm
