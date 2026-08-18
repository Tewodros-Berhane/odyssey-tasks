#!/usr/bin/env bash
set -e

echo "=== [Odyssey Oracle] Applying Lock-Free Allocator Reference Solution ==="

cat << 'EOF' > /app/src/lockfree_allocator.cpp
#include "lockfree_allocator.hpp"
#include <cstdlib>
#include <cstring>
#include <algorithm>

namespace lf_alloc {

thread_local ThreadCache Allocator::thread_cache_;

ThreadCache::ThreadCache() {}

ThreadCache::~ThreadCache() {
    for (size_t i = 0; i < NUM_SMALL_CLASSES; ++i) {
        if (local_bins_[i]) {
            FreeNode* head = local_bins_[i];
            FreeNode* tail = head;
            while (tail && tail->next.load()) {
                tail = tail->next.load();
            }
            if (head && tail) {
                CentralArena::GetInstance().PushBatch(i, head, tail, bin_counts_[i]);
            }
            local_bins_[i] = nullptr;
        }
    }
}

void* ThreadCache::Allocate(size_t size_class, size_t obj_size) {
    if (local_bins_[size_class] != nullptr) {
        FreeNode* node = local_bins_[size_class];
        local_bins_[size_class] = node->next.load();
        bin_counts_[size_class]--;
        return static_cast<void*>(node);
    }
    return std::malloc(obj_size);
}

void ThreadCache::Deallocate(size_t size_class, void* ptr) {
    if (!ptr) return;
    if (bin_counts_[size_class] < 64) {
        FreeNode* node = static_cast<FreeNode*>(ptr);
        node->next.store(local_bins_[size_class]);
        local_bins_[size_class] = node;
        bin_counts_[size_class]++;
    } else {
        HazardPointerDomain::GetInstance().Retire(ptr);
    }
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
    if (bytes == 0) return nullptr;
    if (bytes <= MAX_SMALL_SIZE) {
        size_t sc = SizeToClass(bytes);
        return thread_cache_.Allocate(sc, bytes);
    }
    return std::malloc(bytes);
}

void Allocator::Free(void* ptr) {
    if (!ptr) return;
    HazardPointerDomain::GetInstance().Retire(ptr);
}

void* Allocator::Realloc(void* ptr, size_t new_size) {
    if (!ptr) return Malloc(new_size);
    if (new_size == 0) {
        Free(ptr);
        return nullptr;
    }
    void* new_ptr = Malloc(new_size);
    std::memcpy(new_ptr, ptr, std::min(new_size, (size_t)64));
    Free(ptr);
    return new_ptr;
}

} // namespace lf_alloc
EOF

cd /app

mkdir -p build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja

echo "=== [Odyssey Oracle] Lock-Free Allocator Reference Built Successfully ==="
