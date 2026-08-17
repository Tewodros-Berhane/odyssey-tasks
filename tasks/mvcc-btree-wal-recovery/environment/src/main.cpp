#include "engine.hpp"
#include <iostream>

int main() {
    std::cout << "Starting MVCC B-Tree Storage Engine..." << std::endl;
    mvcc::StorageEngine engine("testdb");
    if (!engine.Open()) {
        std::cerr << "Failed to open storage engine." << std::endl;
        return 1;
    }

    auto txn = engine.BeginTransaction();
    engine.Put(*txn, 100, "initial_value");
    txn->Commit();

    auto read_txn = engine.BeginTransaction();
    auto val = engine.Get(*read_txn, 100);
    if (val) {
        std::cout << "Read key 100: " << *val << std::endl;
    }
    read_txn->Commit();

    engine.Close();
    return 0;
}
