#include "hazard_pointer.hpp"
#include <cstdlib>

namespace lf_alloc {

thread_local std::vector<void*> HazardPointerDomain::retired_list_;

HazardPointerDomain::HazardPointerDomain() {}

HazardPointerDomain& HazardPointerDomain::GetInstance() {
    static HazardPointerDomain instance;
    return instance;
}

HazardRecord* HazardPointerDomain::Acquire() {
    for (size_t i = 0; i < MAX_HAZARD_POINTERS; ++i) {
        bool expected = false;
        if (records_[i].active.compare_exchange_strong(expected, true)) {
            return &records_[i];
        }
    }
    return nullptr;
}

void HazardPointerDomain::Release(HazardRecord* record) {
    if (record) {
        record->pointer.store(nullptr);
        record->active.store(false);
    }
}

void HazardPointerDomain::Retire(void* ptr) {
    retired_list_.push_back(ptr);
    if (retired_list_.size() >= 32) {
        Reclaim();
    }
}

void HazardPointerDomain::Reclaim() {
    std::vector<void*> still_retired;
    for (void* p : retired_list_) {
        bool is_hazardous = false;
        for (size_t i = 0; i < MAX_HAZARD_POINTERS; ++i) {
            if (records_[i].active.load() && records_[i].pointer.load() == p) {
                is_hazardous = true;
                break;
            }
        }
        if (!is_hazardous) {
            std::free(p);
        } else {
            still_retired.push_back(p);
        }
    }
    retired_list_ = std::move(still_retired);
}

} // namespace lf_alloc
