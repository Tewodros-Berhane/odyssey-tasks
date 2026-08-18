#include "lockfree_allocator.hpp"
#include <cstdlib>
#include <cstring>
#include <algorithm>

namespace lf_alloc {

thread_local ThreadCache Allocator::thread_cache_;

ThreadCache::ThreadCache() {}

ThreadCache::~ThreadCache() {}

void* ThreadCache::Allocate(size_t size_class, size_t obj_size) {
    (void)size_class;
    (void)obj_size;
    // TODO: Implement thread-local bin allocation with central batch filling
    return nullptr;
}

void ThreadCache::Deallocate(size_t size_class, void* ptr) {
    (void)size_class;
    (void)ptr;
    // TODO: Implement thread-local bin deallocation with hazard pointer retiring
}

size_t Allocator::SizeToClass(size_t bytes) {
    if (bytes <= 16) return 0;
    if (bytes <= 32) return 1;
    if (bytes <= 64) return 2;
    if (bytes <= 128) return 3;
    if (bytes <= 256) return 4;
    if (bytes <= 512) return 5;
    if (bytes <= 1024) return 6;
    return NUM_SMALL_CLASSES - 1;
}

size_t Allocator::ClassToSize(size_t size_class) {
    return (1ULL << (size_class + 4));
}

void* Allocator::Malloc(size_t bytes) {
    (void)bytes;
    // TODO: Implement Allocator Malloc
    return nullptr;
}

void Allocator::Free(void* ptr) {
    (void)ptr;
    // TODO: Implement Allocator Free
}

void* Allocator::Realloc(void* ptr, size_t new_size) {
    (void)ptr;
    (void)new_size;
    return nullptr;
}

} // namespace lf_alloc
