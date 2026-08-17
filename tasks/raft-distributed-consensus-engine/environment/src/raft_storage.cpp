#include "raft_storage.hpp"

namespace raft {

MemoryStorage::MemoryStorage() {
    LogEntry dummy;
    dummy.term = 0;
    dummy.index = 0;
    log_.push_back(dummy);
}

term_t MemoryStorage::GetCurrentTerm() {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_term_;
}

std::optional<node_id_t> MemoryStorage::GetVotedFor() {
    std::lock_guard<std::mutex> lock(mutex_);
    return voted_for_;
}

void MemoryStorage::SaveHardState(term_t term, std::optional<node_id_t> voted_for) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_term_ = term;
    voted_for_ = voted_for;
}

index_t MemoryStorage::GetLastIndex() {
    std::lock_guard<std::mutex> lock(mutex_);
    return log_.back().index;
}

term_t MemoryStorage::GetLastTerm() {
    std::lock_guard<std::mutex> lock(mutex_);
    return log_.back().term;
}

term_t MemoryStorage::GetTerm(index_t index) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (index == snapshot_index_) return snapshot_term_;
    if (index < snapshot_index_ || index > log_.back().index) return 0;
    size_t offset = index - snapshot_index_;
    if (offset < log_.size()) return log_[offset].term;
    return 0;
}

void MemoryStorage::Append(const std::vector<LogEntry>& entries) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& e : entries) {
        log_.push_back(e);
    }
}

void MemoryStorage::Truncate(index_t from_index) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (from_index <= snapshot_index_) return;
    size_t offset = from_index - snapshot_index_;
    if (offset < log_.size()) {
        log_.erase(log_.begin() + offset, log_.end());
    }
}

std::vector<LogEntry> MemoryStorage::GetEntries(index_t start_index, size_t max_count) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<LogEntry> result;
    if (start_index <= snapshot_index_) return result;
    size_t offset = start_index - snapshot_index_;
    for (size_t i = offset; i < log_.size() && result.size() < max_count; ++i) {
        result.push_back(log_[i]);
    }
    return result;
}

void MemoryStorage::ApplySnapshot(index_t last_included_index, term_t last_included_term, const std::string& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_index_ = last_included_index;
    snapshot_term_ = last_included_term;
    snapshot_data_ = data;
    log_.clear();
    LogEntry dummy;
    dummy.index = last_included_index;
    dummy.term = last_included_term;
    log_.push_back(dummy);
}

} // namespace raft
