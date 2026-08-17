#pragma once
#include "wasm_types.hpp"
#include <memory>
#include <sys/mman.h>

namespace wasm {

using NativeFuncPtr = int64_t (*)(const int64_t* args, uint8_t* memory_base);

class ExecutableBuffer {
public:
    explicit ExecutableBuffer(size_t size = 65536);
    ~ExecutableBuffer();

    void EmitBytes(std::span<const uint8_t> bytes);
    void EmitByte(uint8_t byte);
    void Emit32(uint32_t val);
    void Emit64(uint64_t val);

    NativeFuncPtr GetFunctionPointer(size_t offset = 0);
    size_t GetCurrentOffset() const { return offset_; }

private:
    uint8_t* memory_{nullptr};
    size_t capacity_{0};
    size_t offset_{0};
};

class JITCompiler {
public:
    static std::shared_ptr<ExecutableBuffer> CompileFunction(const Module& module, size_t func_idx);
};

} // namespace wasm
