#include "wasm_runtime.hpp"
#include <iostream>

namespace wasm {

RuntimeInstance::RuntimeInstance(const Module& module)
    : module_(module) {
    memory_.resize(module.initial_memory_pages * 65536, 0);

    for (const auto& exp : module_.exports) {
        if (exp.kind == 0) { // Function export
            auto code_buf = JITCompiler::CompileFunction(module_, exp.index);
            compiled_exports_[exp.name] = code_buf;
        }
    }
}

int64_t RuntimeInstance::Invoke(const std::string& export_name, const std::vector<int64_t>& args) {
    auto it = compiled_exports_.find(export_name);
    if (it == compiled_exports_.end()) return -1;

    NativeFuncPtr fn = it->second->GetFunctionPointer();
    return fn(args.data(), memory_.data());
}

} // namespace wasm
