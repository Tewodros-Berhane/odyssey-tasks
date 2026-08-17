#pragma once
#include "engine.hpp"
#include <fstream>
#include <mutex>
#include <condition_variable>
#include <vector>

namespace mvcc {

class WALManager {
public:
    explicit WALManager(const std::string& log_file_path);
    ~WALManager();

    bool Open();
    void Close();

    lsn_t AppendRecord(LogRecord record);
    void Flush(lsn_t lsn);
    std::vector<LogRecord> ReadAllRecords();
    void Truncate(lsn_t up_to_lsn);

    lsn_t GetFlushedLSN() const { return flushed_lsn_.load(); }

private:
    std::string log_file_path_;
    std::fstream log_file_;
    mutable std::mutex wal_mutex_;
    std::condition_variable cv_;
    std::atomic<lsn_t> next_lsn_{1};
    std::atomic<lsn_t> flushed_lsn_{0};
};

} // namespace mvcc
