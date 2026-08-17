#!/usr/bin/env bash
set -e

echo "=== [Odyssey Oracle] Applying Reference Solution ==="

cd /app

cat << 'EOF' > src/engine.cpp
#include "engine.hpp"
#include "wal.hpp"
#include "transaction.hpp"
#include <iostream>
#include <unordered_map>
#include <unordered_set>

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
    std::unique_lock lock(index_mutex_);
    auto records = wal_->ReadAllRecords();
    if (records.empty()) return true;

    // Phase 1: Analysis - Find active transactions and max LSN
    std::unordered_map<txn_id_t, lsn_t> active_txns; // txn_id -> last_lsn
    timestamp_t max_ts = 1;

    for (const auto& rec : records) {
        if (rec.type == LogType::BEGIN) {
            active_txns[rec.txn_id] = rec.lsn;
        } else if (rec.type == LogType::COMMIT || rec.type == LogType::ABORT) {
            active_txns.erase(rec.txn_id);
        } else if (rec.type == LogType::INSERT || rec.type == LogType::UPDATE || rec.type == LogType::DELETE) {
            active_txns[rec.txn_id] = rec.lsn;
        }
        if (rec.txn_id > next_txn_id_.load()) {
            next_txn_id_.store(rec.txn_id + 1);
        }
    }

    // Phase 2: Redo - Repeat history
    for (const auto& rec : records) {
        if (rec.type == LogType::INSERT || rec.type == LogType::UPDATE) {
            auto vrec = std::make_shared<VersionRecord>();
            vrec->commit_ts = rec.lsn;
            vrec->txn_id = rec.txn_id;
            vrec->is_deleted = false;
            vrec->value = rec.after_val;
            if (index_.count(rec.key)) {
                vrec->prev = index_[rec.key];
            }
            index_[rec.key] = vrec;
            max_ts = std::max(max_ts, rec.lsn);
        } else if (rec.type == LogType::DELETE) {
            auto vrec = std::make_shared<VersionRecord>();
            vrec->commit_ts = rec.lsn;
            vrec->txn_id = rec.txn_id;
            vrec->is_deleted = true;
            if (index_.count(rec.key)) {
                vrec->prev = index_[rec.key];
            }
            index_[rec.key] = vrec;
            max_ts = std::max(max_ts, rec.lsn);
        }
    }

    // Phase 3: Undo - Roll back uncommitted (active) transactions
    for (const auto& [tid, last_lsn] : active_txns) {
        for (auto it = index_.begin(); it != index_.end(); ++it) {
            auto curr = it->second;
            std::shared_ptr<VersionRecord> prev_valid = nullptr;
            std::shared_ptr<VersionRecord> head = curr;

            while (head && head->txn_id == tid) {
                head = head->prev;
            }
            if (head != curr) {
                if (head) {
                    it->second = head;
                } else {
                    it->second = nullptr;
                }
            }
        }
        
        LogRecord clr;
        clr.txn_id = tid;
        clr.type = LogType::ABORT;
        wal_->AppendRecord(clr);
    }

    global_ts_.store(max_ts + 1);
    return true;
}

void StorageEngine::Checkpoint() {
    std::unique_lock lock(index_mutex_);
    LogRecord rec;
    rec.type = LogType::CHECKPOINT;
    lsn_t lsn = wal_->AppendRecord(rec);
    wal_->Flush(lsn);
}

void StorageEngine::Vacuum() {
    std::unique_lock lock(index_mutex_);
    timestamp_t min_active_ts = global_ts_.load();
    for (auto& [k, head] : index_) {
        auto curr = head;
        std::shared_ptr<VersionRecord> prev = nullptr;
        while (curr) {
            if (curr->commit_ts < min_active_ts && curr->prev) {
                if (curr->is_deleted) {
                    if (prev) prev->prev = nullptr;
                    else head = nullptr;
                } else {
                    curr->prev = nullptr;
                }
                break;
            }
            prev = curr;
            curr = curr->prev;
        }
    }
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
EOF

mkdir -p build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja

echo "=== [Odyssey Oracle] Reference Solution Built Successfully ==="
