#pragma once
#include "raft_types.hpp"
#include <mutex>
#include <vector>

namespace raft {

class Storage {
public:
    virtual ~Storage() = default;

    virtual term_t GetCurrentTerm() = 0;
    virtual std::optional<node_id_t> GetVotedFor() = 0;
    virtual void SaveHardState(term_t term, std::optional<node_id_t> voted_for) = 0;

    virtual index_t GetLastIndex() = 0;
    virtual term_t GetLastTerm() = 0;
    virtual term_t GetTerm(index_t index) = 0;

    virtual void Append(const std::vector<LogEntry>& entries) = 0;
    virtual void Truncate(index_t from_index) = 0;
    virtual std::vector<LogEntry> GetEntries(index_t start_index, size_t max_count) = 0;

    virtual void ApplySnapshot(index_t last_included_index, term_t last_included_term, const std::string& data) = 0;
};

class MemoryStorage : public Storage {
public:
    MemoryStorage();

    term_t GetCurrentTerm() override;
    std::optional<node_id_t> GetVotedFor() override;
    void SaveHardState(term_t term, std::optional<node_id_t> voted_for) override;

    index_t GetLastIndex() override;
    term_t GetLastTerm() override;
    term_t GetTerm(index_t index) override;

    void Append(const std::vector<LogEntry>& entries) override;
    void Truncate(index_t from_index) override;
    std::vector<LogEntry> GetEntries(index_t start_index, size_t max_count) override;

    void ApplySnapshot(index_t last_included_index, term_t last_included_term, const std::string& data) override;

private:
    std::mutex mutex_;
    term_t current_term_{0};
    std::optional<node_id_t> voted_for_{std::nullopt};
    std::vector<LogEntry> log_;
    index_t snapshot_index_{0};
    term_t snapshot_term_{0};
    std::string snapshot_data_;
};

} // namespace raft
