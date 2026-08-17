#include "hnsw.hpp"
#include <iostream>

int main() {
    std::cout << "Starting AVX2/AVX-512 HNSW Vector Engine..." << std::endl;
    vecengine::HNSWIndex index(768, 32, 128, 64);
    std::cout << "Index initialized successfully." << std::endl;
    return 0;
}
