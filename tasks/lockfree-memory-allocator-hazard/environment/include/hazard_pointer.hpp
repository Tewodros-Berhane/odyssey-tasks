#pragma once
#include <atomic>
#include <vector>
#include <cstdint>

namespace lf_alloc {

constexpr size_t MAX_HAZARD_POINTERS = 128;

struct alignas(64) HazardRecord {
    std::atomic<void*> pointer{nullptr};
    std::atomic<bool> active{false};
};

class HazardPointerDomain {
public:
    static HazardPointerDomain& GetInstance();

    HazardRecord* Acquire();
    void Release(HazardRecord* record);
    void Retire(void* ptr);
    void Reclaim();

private:
    HazardPointerDomain();
    HazardRecord records_[MAX_HAZARD_POINTERS];
    thread_local static std::vector<void*> retired_list_;
};

} // namespace lf_alloc
