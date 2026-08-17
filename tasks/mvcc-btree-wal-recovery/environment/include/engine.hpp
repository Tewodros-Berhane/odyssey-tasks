#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <shared_mutex>
#include <atomic>
#include <map>

namespace mvcc {

using txn_id_t = uint64_t;
using lsn_t = uint64_t;
using timestamp_t = uint64_t;
using key_t = int64_t;
using val_t = std::string;

constexpr lsn_t INVALID_LSN = 0;
constexpr txn_id_t INVALID_TXN_ID = 0;

enum class LogType : uint8_t {
    INVALID = 0,
    BEGIN,
    INSERT,
    UPDATE,
    DELETE,
    COMMIT,
    ABORT,
    CLR,
    CHECKPOINT
};

struct VersionRecord {
    timestamp_t commit_ts{0};
    txn_id_t txn_id{0};
    bool is_deleted{false};
    val_t value;
    std::shared_ptr<VersionRecord> prev{nullptr};
};

struct LogRecord {
    lsn_t lsn{INVALID_LSN};
    lsn_t prev_lsn{INVALID_LSN};
    txn_id_t txn_id{INVALID_TXN_ID};
    LogType type{LogType::INVALID};
    key_t key{0};
    val_t before_val;
    val_t after_val;
    lsn_t undo_next_lsn{INVALID_LSN};
};

class Transaction;
class WALManager;

class StorageEngine {
public:
    explicit StorageEngine(const std::string& db_path);
    ~StorageEngine();

    bool Open();
    void Close();

    std::unique_ptr<Transaction> BeginTransaction();
    bool Recover();
    void Checkpoint();
    void Vacuum();

    bool Put(Transaction& txn, key_t key, const val_t& value);
    std::optional<val_t> Get(Transaction& txn, key_t key);
    bool Delete(Transaction& txn, key_t key);
    std::vector<std::pair<key_t, val_t>> Scan(Transaction& txn, key_t start_key, key_t end_key);

    std::shared_ptr<WALManager> GetWAL() { return wal_; }

private:
    std::string db_path_;
    std::shared_ptr<WALManager> wal_;
    std::atomic<timestamp_t> global_ts_{1};
    std::atomic<txn_id_t> next_txn_id_{1};
    std::shared_mutex index_mutex_;
    std::map<key_t, std::shared_ptr<VersionRecord>> index_;
};

} // namespace mvcc
