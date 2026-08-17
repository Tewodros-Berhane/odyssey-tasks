#pragma once
#include "raft_node.hpp"
#include <unordered_map>
#include <set>

namespace raft {

class RaftCluster {
public:
    RaftCluster() = default;

    void AddNode(std::shared_ptr<RaftNode> node);
    void SetPartition(const std::set<node_id_t>& group_a, const std::set<node_id_t>& group_b);
    void ClearPartitions();

    void SendRequestVote(node_id_t from, node_id_t to, const RequestVoteArgs& args);
    void SendAppendEntries(node_id_t from, node_id_t to, const AppendEntriesArgs& args);
    void SendInstallSnapshot(node_id_t from, node_id_t to, const InstallSnapshotArgs& args);

    void TickAll();

private:
    bool CanCommunicate(node_id_t a, node_id_t b);

    std::unordered_map<node_id_t, std::shared_ptr<RaftNode>> nodes_;
    std::mutex cluster_mutex_;
    std::set<std::pair<node_id_t, node_id_t>> blocked_links_;
};

} // namespace raft
