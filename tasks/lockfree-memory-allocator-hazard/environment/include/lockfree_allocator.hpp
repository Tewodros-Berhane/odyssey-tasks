#pragma once
#include "allocator_types.hpp"
#include "hazard_pointer.hpp"
#include "slab_arena.hpp"

namespace lf_alloc {

class ThreadCache {
public:
    ThreadCache();
    ~ThreadCache();

    void* Allocate(size_t size_class, size_t obj_size);
    void Deallocate(size_t size_class, void* ptr);

private:
    FreeNode* local_bins_[NUM_SMALL_CLASSES]{nullptr};
    size_t bin_counts_[NUM_SMALL_CLASSES]{0};
};

class Allocator {
public:
    static void* Malloc(size_t bytes);
    static void Free(void* ptr);
    static void* Realloc(void* ptr, size_t new_size);

    static size_t SizeToClass(size_t bytes);
    static size_t ClassToSize(size_t size_class);

private:
    thread_local static ThreadCache thread_cache_;
};

} // namespace lf_alloc
