#include "storage.hpp"
#include <cassert>
#include <iostream>

int main() {
    db::BTree tree;
    tree.Insert(42, 100);
    int val = tree.Get(42);
    if (val != 100) {
        std::cerr << "Failed: Expected 100, got " << val << std::endl;
        return 1;
    }
    std::cout << "Basic test passed!" << std::endl;
    return 0;
}
