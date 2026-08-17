#include "wal.hpp"
#include <iostream>

namespace mvcc {

WALManager::WALManager(const std::string& log_file_path)
    : log_file_path_(log_file_path) {}

WALManager::~WALManager() {
    Close();
}

bool WALManager::Open() {
    std::lock_guard<std::mutex> lock(wal_mutex_);
    log_file_.open(log_file_path_, std::ios::in | std::ios::out | std::ios::app | std::ios::binary);
    if (!log_file_.is_open()) {
        log_file_.open(log_file_path_, std::ios::out | std::ios::binary);
        log_file_.close();
        log_file_.open(log_file_path_, std::ios::in | std::ios::out | std::ios::app | std::ios::binary);
    }
    return log_file_.is_open();
}

void WALManager::Close() {
    std::lock_guard<std::mutex> lock(wal_mutex_);
    if (log_file_.is_open()) {
        log_file_.flush();
        log_file_.close();
    }
}

lsn_t WALManager::AppendRecord(LogRecord record) {
    std::lock_guard<std::mutex> lock(wal_mutex_);
    lsn_t lsn = next_lsn_.fetch_add(1);
    record.lsn = lsn;
    if (log_file_.is_open()) {
        log_file_.write(reinterpret_cast<const char*>(&record.lsn), sizeof(record.lsn));
        log_file_.write(reinterpret_cast<const char*>(&record.prev_lsn), sizeof(record.prev_lsn));
        log_file_.write(reinterpret_cast<const char*>(&record.txn_id), sizeof(record.txn_id));
        log_file_.write(reinterpret_cast<const char*>(&record.type), sizeof(record.type));
        log_file_.write(reinterpret_cast<const char*>(&record.key), sizeof(record.key));
        log_file_.write(reinterpret_cast<const char*>(&record.undo_next_lsn), sizeof(record.undo_next_lsn));
        
        uint32_t len_before = record.before_val.size();
        log_file_.write(reinterpret_cast<const char*>(&len_before), sizeof(len_before));
        if (len_before > 0) log_file_.write(record.before_val.data(), len_before);

        uint32_t len_after = record.after_val.size();
        log_file_.write(reinterpret_cast<const char*>(&len_after), sizeof(len_after));
        if (len_after > 0) log_file_.write(record.after_val.data(), len_after);
    }
    return lsn;
}

void WALManager::Flush(lsn_t lsn) {
    std::lock_guard<std::mutex> lock(wal_mutex_);
    if (log_file_.is_open()) {
        log_file_.flush();
    }
    flushed_lsn_.store(lsn);
    cv_.notify_all();
}

std::vector<LogRecord> WALManager::ReadAllRecords() {
    std::lock_guard<std::mutex> lock(wal_mutex_);
    std::vector<LogRecord> records;
    if (!log_file_.is_open()) return records;

    log_file_.seekg(0, std::ios::beg);
    while (log_file_.peek() != EOF) {
        LogRecord rec;
        if (!log_file_.read(reinterpret_cast<char*>(&rec.lsn), sizeof(rec.lsn))) break;
        log_file_.read(reinterpret_cast<char*>(&rec.prev_lsn), sizeof(rec.prev_lsn));
        log_file_.read(reinterpret_cast<char*>(&rec.txn_id), sizeof(rec.txn_id));
        log_file_.read(reinterpret_cast<char*>(&rec.type), sizeof(rec.type));
        log_file_.read(reinterpret_cast<char*>(&rec.key), sizeof(rec.key));
        log_file_.read(reinterpret_cast<char*>(&rec.undo_next_lsn), sizeof(rec.undo_next_lsn));

        uint32_t len_before = 0;
        log_file_.read(reinterpret_cast<char*>(&len_before), sizeof(len_before));
        if (len_before > 0) {
            rec.before_val.resize(len_before);
            log_file_.read(&rec.before_val[0], len_before);
        }

        uint32_t len_after = 0;
        log_file_.read(reinterpret_cast<char*>(&len_after), sizeof(len_after));
        if (len_after > 0) {
            rec.after_val.resize(len_after);
            log_file_.read(&rec.after_val[0], len_after);
        }

        records.push_back(rec);
    }
    log_file_.clear();
    log_file_.seekp(0, std::ios::end);
    return records;
}

void WALManager::Truncate(lsn_t) {
    // SKELETON
}

} // namespace mvcc
