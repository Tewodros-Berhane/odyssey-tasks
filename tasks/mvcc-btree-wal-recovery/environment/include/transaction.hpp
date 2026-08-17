#pragma once
#include "engine.hpp"
#include <unordered_map>
#include <unordered_set>

namespace mvcc {

enum class TxnState {
    ACTIVE,
    COMMITTED,
    ABORTED
};

class Transaction {
public:
    Transaction(txn_id_t txn_id, timestamp_t read_ts, StorageEngine* engine);
    ~Transaction();

    txn_id_t GetTxnId() const { return txn_id_; }
    timestamp_t GetReadTs() const { return read_ts_; }
    timestamp_t GetCommitTs() const { return commit_ts_; }
    TxnState GetState() const { return state_; }
    lsn_t GetPrevLSN() const { return prev_lsn_; }
    void SetPrevLSN(lsn_t lsn) { prev_lsn_ = lsn; }

    bool Commit();
    void Abort();

    void RecordWrite(key_t key, const std::optional<val_t>& before_val, const std::optional<val_t>& after_val);

private:
    txn_id_t txn_id_;
    timestamp_t read_ts_;
    timestamp_t commit_ts_{0};
    TxnState state_{TxnState::ACTIVE};
    lsn_t prev_lsn_{INVALID_LSN};
    StorageEngine* engine_;
    std::unordered_map<key_t, std::optional<val_t>> write_set_;
};

} // namespace mvcc
