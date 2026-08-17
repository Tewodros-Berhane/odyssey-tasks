#include "wasm_jit.hpp"
#include <cstring>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

namespace wasm {

ExecutableBuffer::ExecutableBuffer(size_t size) : capacity_(size) {
#ifdef _WIN32
    memory_ = static_cast<uint8_t*>(VirtualAlloc(NULL, capacity_, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
#else
    memory_ = static_cast<uint8_t*>(mmap(NULL, capacity_, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0));
#endif
}

ExecutableBuffer::~ExecutableBuffer() {
    if (memory_) {
#ifdef _WIN32
        VirtualFree(memory_, 0, MEM_RELEASE);
#else
        munmap(memory_, capacity_);
#endif
    }
}

void ExecutableBuffer::EmitByte(uint8_t byte) {
    if (offset_ < capacity_) {
        memory_[offset_++] = byte;
    }
}

void ExecutableBuffer::EmitBytes(std::span<const uint8_t> bytes) {
    for (uint8_t b : bytes) EmitByte(b);
}

void ExecutableBuffer::Emit32(uint32_t val) {
    for (int i = 0; i < 4; ++i) {
        EmitByte(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
    }
}

NativeFuncPtr ExecutableBuffer::GetFunctionPointer(size_t offset) {
    return reinterpret_cast<NativeFuncPtr>(memory_ + offset);
}

std::shared_ptr<ExecutableBuffer> JITCompiler::CompileFunction(const Module&, size_t) {
    auto buf = std::make_shared<ExecutableBuffer>(4096);
    // Emit minimal x86-64 prologue & epilogue:
    // mov rax, [rdi] ; return args[0]
    // ret
    buf->EmitByte(0x48); buf->EmitByte(0x8B); buf->EmitByte(0x07); // mov rax, [rdi]
    buf->EmitByte(0xC3); // ret
    return buf;
}

} // namespace wasm
