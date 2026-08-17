#pragma once
#include "raft_types.hpp"
#include "raft_storage.hpp"
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <mutex>

namespace raft {

class RaftCluster;

class RaftNode {
public:
    RaftNode(node_id_t id, std::vector<node_id_t> peers, std::shared_ptr<Storage> storage, RaftCluster* cluster);

    node_id_t GetId() const { return id_; }
    Role GetRole() const { return role_; }
    term_t GetTerm() const;
    index_t GetCommitIndex() const { return commit_index_; }
    index_t GetLastApplied() const { return last_applied_; }

    void HandleTick();
    RequestVoteReply HandleRequestVote(const RequestVoteArgs& args);
    AppendEntriesReply HandleAppendEntries(const AppendEntriesArgs& args);
    InstallSnapshotReply HandleInstallSnapshot(const InstallSnapshotArgs& args);

    bool Propose(const std::string& data);
    void StepDown(term_t new_term);

private:
    void StartPreVote();
    void StartElection();
    void BroadcastHeartbeats();
    void BroadcastAppendEntries();

    node_id_t id_;
    std::vector<node_id_t> peers_;
    std::shared_ptr<Storage> storage_;
    RaftCluster* cluster_;

    Role role_{Role::FOLLOWER};
    mutable std::mutex node_mutex_;

    index_t commit_index_{0};
    index_t last_applied_{0};

    int election_elapsed_{0};
    int heartbeat_elapsed_{0};
    int randomized_election_timeout_{15};
    int heartbeat_timeout_{5};

    std::unordered_set<node_id_t> votes_granted_;
    std::unordered_map<node_id_t, index_t> next_index_;
    std::unordered_map<node_id_t, index_t> match_index_;
};

} // namespace raft
