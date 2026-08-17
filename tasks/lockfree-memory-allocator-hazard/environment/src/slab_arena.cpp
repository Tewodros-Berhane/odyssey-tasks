#include "slab_arena.hpp"
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

namespace lf_alloc {

CentralArena::CentralArena() {
    for (size_t i = 0; i < NUM_SMALL_CLASSES; ++i) {
        central_bins_[i].store(nullptr);
    }
}

CentralArena& CentralArena::GetInstance() {
    static CentralArena instance;
    return instance;
}

void* CentralArena::AllocateSpan(size_t num_bytes) {
#ifdef _WIN32
    return VirtualAlloc(NULL, num_bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    return mmap(NULL, num_bytes, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
#endif
}

void CentralArena::FreeSpan(void* ptr, size_t num_bytes) {
    if (!ptr) return;
#ifdef _WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    munmap(ptr, num_bytes);
#endif
}

FreeNode* CentralArena::PopBatch(size_t size_class, size_t count) {
    if (size_class >= NUM_SMALL_CLASSES) return nullptr;
    FreeNode* head = central_bins_[size_class].load();
    while (head && !central_bins_[size_class].compare_exchange_weak(head, head->next.load())) {
        // CAS retry
    }
    return head;
}

void CentralArena::PushBatch(size_t size_class, FreeNode* head, FreeNode* tail, size_t count) {
    if (!head || size_class >= NUM_SMALL_CLASSES) return;
    FreeNode* old_head = central_bins_[size_class].load();
    do {
        tail->next.store(old_head);
    } while (!central_bins_[size_class].compare_exchange_weak(old_head, head));
}

} // namespace lf_alloc
