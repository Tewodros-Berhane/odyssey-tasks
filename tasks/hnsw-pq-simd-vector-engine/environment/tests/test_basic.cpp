#include "simd_ops.hpp"
#include "pq.hpp"
#include "hnsw.hpp"
#include <cassert>
#include <iostream>
#include <vector>

void TestSIMDAndPQ() {
    std::vector<float> a(768, 1.0f);
    std::vector<float> b(768, 1.0f);
    float l2 = vecengine::SIMDOps::L2Distance(a, b);
    assert(l2 == 0.0f);

    vecengine::HNSWIndex index(768, 32, 64, 32);
    index.Insert(0, a);
    auto res = index.SearchKNN(a, 1);
    assert(!res.empty());
    assert(res[0].id == 0);

    std::cout << "TestSIMDAndPQ passed!" << std::endl;
}

int main() {
    TestSIMDAndPQ();
    return 0;
}
