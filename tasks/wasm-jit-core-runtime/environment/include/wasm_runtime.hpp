#pragma once
#include "wasm_types.hpp"
#include "wasm_jit.hpp"
#include <unordered_map>
#include <vector>

namespace wasm {

class RuntimeInstance {
public:
    explicit RuntimeInstance(const Module& module);

    int64_t Invoke(const std::string& export_name, const std::vector<int64_t>& args);
    uint8_t* GetMemoryBase() { return memory_.data(); }

private:
    Module module_;
    std::vector<uint8_t> memory_;
    std::unordered_map<std::string, std::shared_ptr<ExecutableBuffer>> compiled_exports_;
};

} // namespace wasm
