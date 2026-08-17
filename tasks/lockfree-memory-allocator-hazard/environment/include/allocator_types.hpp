#pragma once
#include <cstdint>
#include <cstddef>
#include <atomic>
#include <vector>

namespace lf_alloc {

constexpr size_t PAGE_SIZE = 4096;
constexpr size_t SLAB_SIZE = 65536; // 64 KiB
constexpr size_t MAX_SMALL_SIZE = 1024;
constexpr size_t NUM_SMALL_CLASSES = 13;

struct alignas(64) FreeNode {
    std::atomic<FreeNode*> next{nullptr};
};

struct SlabHeader {
    size_t size_class;
    size_t obj_size;
    std::atomic<size_t> allocated_count{0};
    std::atomic<FreeNode*> free_list{nullptr};
};

} // namespace lf_alloc
