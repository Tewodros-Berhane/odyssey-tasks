#pragma once
#include "allocator_types.hpp"
#include <mutex>
#include <vector>

namespace lf_alloc {

class CentralArena {
public:
    static CentralArena& GetInstance();

    void* AllocateSpan(size_t num_bytes);
    void FreeSpan(void* ptr, size_t num_bytes);

    FreeNode* PopBatch(size_t size_class, size_t count);
    void PushBatch(size_t size_class, FreeNode* head, FreeNode* tail, size_t count);

private:
    CentralArena();
    std::atomic<FreeNode*> central_bins_[NUM_SMALL_CLASSES];
};

} // namespace lf_alloc
