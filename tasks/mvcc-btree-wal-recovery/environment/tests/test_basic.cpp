#include "engine.hpp"
#include <cassert>
#include <iostream>

void TestBasicCRUD() {
    mvcc::StorageEngine engine("basic_test.db");
    assert(engine.Open());

    auto txn1 = engine.BeginTransaction();
    assert(engine.Put(*txn1, 1, "value_1"));
    assert(engine.Put(*txn1, 2, "value_2"));
    assert(txn1->Commit());

    auto txn2 = engine.BeginTransaction();
    auto v1 = engine.Get(*txn2, 1);
    assert(v1.has_value() && *v1 == "value_1");
    auto v2 = engine.Get(*txn2, 2);
    assert(v2.has_value() && *v2 == "value_2");
    assert(txn2->Commit());

    std::cout << "TestBasicCRUD passed!" << std::endl;
}

int main() {
    TestBasicCRUD();
    return 0;
}
