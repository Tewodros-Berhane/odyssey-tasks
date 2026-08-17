#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace raft {

using node_id_t = uint64_t;
using term_t = uint64_t;
using index_t = uint64_t;

enum class Role {
    FOLLOWER,
    PRECANDIDATE,
    CANDIDATE,
    LEADER
};

enum class EntryType {
    NORMAL,
    CONF_CHANGE,
    JOINT_CONF_CHANGE,
    SNAPSHOT
};

struct LogEntry {
    term_t term{0};
    index_t index{0};
    EntryType type{EntryType::NORMAL};
    std::string data;
};

struct RequestVoteArgs {
    term_t term{0};
    node_id_t candidate_id{0};
    index_t last_log_index{0};
    term_t last_log_term{0};
    bool is_pre_vote{false};
};

struct RequestVoteReply {
    term_t term{0};
    bool vote_granted{false};
};

struct AppendEntriesArgs {
    term_t term{0};
    node_id_t leader_id{0};
    index_t prev_log_index{0};
    term_t prev_log_term{0};
    std::vector<LogEntry> entries;
    index_t leader_commit{0};
};

struct AppendEntriesReply {
    term_t term{0};
    bool success{false};
    index_t match_index{0};
};

struct InstallSnapshotArgs {
    term_t term{0};
    node_id_t leader_id{0};
    index_t last_included_index{0};
    term_t last_included_term{0};
    std::string data;
};

struct InstallSnapshotReply {
    term_t term{0};
};

} // namespace raft
