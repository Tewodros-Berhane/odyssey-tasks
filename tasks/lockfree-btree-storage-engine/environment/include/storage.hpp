#pragma once
#include <cstdint>
#include <string>

namespace db {
class BufferPool {
public:
    BufferPool(size_t num_pages);
    ~BufferPool();
    bool ReadPage(int page_id, char* dest);
    bool WritePage(int page_id, const char* src);
};

class BTree {
public:
    BTree();
    ~BTree();
    void Insert(int key, int value);
    int Get(int key);
};

class RecoveryManager {
public:
    RecoveryManager();
    void Recover();
};
}
