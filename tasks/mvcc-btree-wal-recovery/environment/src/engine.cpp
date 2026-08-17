#include "engine.hpp"
#include "wal.hpp"
#include "transaction.hpp"
#include <iostream>

namespace mvcc {

StorageEngine::StorageEngine(const std::string& db_path)
    : db_path_(db_path), wal_(std::make_shared<WALManager>(db_path + ".wal")) {}

StorageEngine::~StorageEngine() {
    Close();
}

bool StorageEngine::Open() {
    return wal_->Open();
}

void StorageEngine::Close() {
    if (wal_) {
        wal_->Close();
    }
}

std::unique_ptr<Transaction> StorageEngine::BeginTransaction() {
    txn_id_t tid = next_txn_id_.fetch_add(1);
    timestamp_t rts = global_ts_.load();
    return std::make_unique<Transaction>(tid, rts, this);
}

bool StorageEngine::Recover() {
    // SKELETON: To be completed according to ARIES specifications
    return true;
}

void StorageEngine::Checkpoint() {
    // SKELETON: To be implemented
}

void StorageEngine::Vacuum() {
    // SKELETON: To be implemented
}

bool StorageEngine::Put(Transaction& txn, key_t key, const val_t& value) {
    std::unique_lock lock(index_mutex_);
    auto rec = std::make_shared<VersionRecord>();
    rec->commit_ts = txn.GetCommitTs() ? txn.GetCommitTs() : UINT64_MAX;
    rec->txn_id = txn.GetTxnId();
    rec->is_deleted = false;
    rec->value = value;
    if (index_.count(key)) {
        rec->prev = index_[key];
    }
    index_[key] = rec;
    txn.RecordWrite(key, std::nullopt, value);
    return true;
}

std::optional<val_t> StorageEngine::Get(Transaction& txn, key_t key) {
    std::shared_lock lock(index_mutex_);
    auto it = index_.find(key);
    if (it == index_.end()) return std::nullopt;

    auto curr = it->second;
    while (curr) {
        if (curr->txn_id == txn.GetTxnId() || curr->commit_ts <= txn.GetReadTs()) {
            if (curr->is_deleted) return std::nullopt;
            return curr->value;
        }
        curr = curr->prev;
    }
    return std::nullopt;
}

bool StorageEngine::Delete(Transaction& txn, key_t key) {
    std::unique_lock lock(index_mutex_);
    auto rec = std::make_shared<VersionRecord>();
    rec->commit_ts = txn.GetCommitTs() ? txn.GetCommitTs() : UINT64_MAX;
    rec->txn_id = txn.GetTxnId();
    rec->is_deleted = true;
    if (index_.count(key)) {
        rec->prev = index_[key];
    }
    index_[key] = rec;
    txn.RecordWrite(key, std::nullopt, std::nullopt);
    return true;
}

std::vector<std::pair<key_t, val_t>> StorageEngine::Scan(Transaction& txn, key_t start_key, key_t end_key) {
    std::shared_lock lock(index_mutex_);
    std::vector<std::pair<key_t, val_t>> results;
    for (auto it = index_.lower_bound(start_key); it != index_.end() && it->first <= end_key; ++it) {
        auto curr = it->second;
        while (curr) {
            if (curr->txn_id == txn.GetTxnId() || curr->commit_ts <= txn.GetReadTs()) {
                if (!curr->is_deleted) {
                    results.emplace_back(it->first, curr->value);
                }
                break;
            }
            curr = curr->prev;
        }
    }
    return results;
}

} // namespace mvcc
