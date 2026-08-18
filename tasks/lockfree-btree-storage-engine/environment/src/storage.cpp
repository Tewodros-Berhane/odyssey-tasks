#include "storage.hpp"

namespace db {

BufferPool::BufferPool(size_t num_pages) { (void)num_pages; }
BufferPool::~BufferPool() {}
bool BufferPool::ReadPage(int page_id, char* dest) { (void)page_id; (void)dest; return false; }
bool BufferPool::WritePage(int page_id, const char* src) { (void)page_id; (void)src; return false; }

BTree::BTree() {}
BTree::~BTree() {}
void BTree::Insert(int key, int value) { (void)key; (void)value; }
int BTree::Get(int key) { (void)key; return -1; }

RecoveryManager::RecoveryManager() {}
void RecoveryManager::Recover() {}

}
