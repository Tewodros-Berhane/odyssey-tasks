#include "lockfree_allocator.hpp"
#include <cassert>
#include <iostream>

void TestBasicAllocation() {
    void* p1 = lf_alloc::Allocator::Malloc(32);
    assert(p1 != nullptr);
    // Alignment check
    assert(reinterpret_cast<uintptr_t>(p1) % 16 == 0);

    void* p2 = lf_alloc::Allocator::Malloc(64);
    assert(p2 != nullptr);

    lf_alloc::Allocator::Free(p1);
    lf_alloc::Allocator::Free(p2);

    std::cout << "TestBasicAllocation passed!" << std::endl;
}

int main() {
    TestBasicAllocation();
    return 0;
}
