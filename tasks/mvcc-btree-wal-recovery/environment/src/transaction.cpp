#include "transaction.hpp"
#include "wal.hpp"

namespace mvcc {

Transaction::Transaction(txn_id_t txn_id, timestamp_t read_ts, StorageEngine* engine)
    : txn_id_(txn_id), read_ts_(read_ts), engine_(engine) {
    if (engine_ && engine_->GetWAL()) {
        LogRecord rec;
        rec.txn_id = txn_id_;
        rec.type = LogType::BEGIN;
        prev_lsn_ = engine_->GetWAL()->AppendRecord(rec);
    }
}

Transaction::~Transaction() {
    if (state_ == TxnState::ACTIVE) {
        Abort();
    }
}

bool Transaction::Commit() {
    if (state_ != TxnState::ACTIVE) return false;
    state_ = TxnState::COMMITTED;
    commit_ts_ = read_ts_ + 1;

    if (engine_ && engine_->GetWAL()) {
        LogRecord rec;
        rec.txn_id = txn_id_;
        rec.prev_lsn = prev_lsn_;
        rec.type = LogType::COMMIT;
        lsn_t lsn = engine_->GetWAL()->AppendRecord(rec);
        engine_->GetWAL()->Flush(lsn);
    }
    return true;
}

void Transaction::Abort() {
    if (state_ != TxnState::ACTIVE) return;
    state_ = TxnState::ABORTED;

    if (engine_ && engine_->GetWAL()) {
        LogRecord rec;
        rec.txn_id = txn_id_;
        rec.prev_lsn = prev_lsn_;
        rec.type = LogType::ABORT;
        lsn_t lsn = engine_->GetWAL()->AppendRecord(rec);
        engine_->GetWAL()->Flush(lsn);
    }
}

void Transaction::RecordWrite(key_t key, const std::optional<val_t>& before_val, const std::optional<val_t>& after_val) {
    write_set_[key] = after_val;
    if (engine_ && engine_->GetWAL()) {
        LogRecord rec;
        rec.txn_id = txn_id_;
        rec.prev_lsn = prev_lsn_;
        rec.key = key;
        if (after_val.has_value() && !before_val.has_value()) {
            rec.type = LogType::INSERT;
            rec.after_val = *after_val;
        } else if (after_val.has_value() && before_val.has_value()) {
            rec.type = LogType::UPDATE;
            rec.before_val = *before_val;
            rec.after_val = *after_val;
        } else {
            rec.type = LogType::DELETE;
            if (before_val.has_value()) rec.before_val = *before_val;
        }
        prev_lsn_ = engine_->GetWAL()->AppendRecord(rec);
    }
}

} // namespace mvcc
