#include "lockfree_allocator.hpp"
#include <iostream>

int main() {
    std::cout << "Starting Lock-Free Memory Allocator..." << std::endl;
    void* p = lf_alloc::Allocator::Malloc(128);
    std::cout << "Allocated 128 bytes at " << p << std::endl;
    lf_alloc::Allocator::Free(p);
    std::cout << "Freed memory." << std::endl;
    return 0;
}
