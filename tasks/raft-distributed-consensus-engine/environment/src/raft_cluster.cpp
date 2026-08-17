#include "raft_cluster.hpp"

namespace raft {

void RaftCluster::AddNode(std::shared_ptr<RaftNode> node) {
    std::lock_guard<std::mutex> lock(cluster_mutex_);
    nodes_[node->GetId()] = node;
}

void RaftCluster::SetPartition(const std::set<node_id_t>& group_a, const std::set<node_id_t>& group_b) {
    std::lock_guard<std::mutex> lock(cluster_mutex_);
    for (node_id_t a : group_a) {
        for (node_id_t b : group_b) {
            blocked_links_.insert({a, b});
            blocked_links_.insert({b, a});
        }
    }
}

void RaftCluster::ClearPartitions() {
    std::lock_guard<std::mutex> lock(cluster_mutex_);
    blocked_links_.clear();
}

bool RaftCluster::CanCommunicate(node_id_t a, node_id_t b) {
    return blocked_links_.find({a, b}) == blocked_links_.end();
}

void RaftCluster::SendRequestVote(node_id_t from, node_id_t to, const RequestVoteArgs& args) {
    std::shared_ptr<RaftNode> target = nullptr;
    {
        std::lock_guard<std::mutex> lock(cluster_mutex_);
        if (!CanCommunicate(from, to)) return;
        auto it = nodes_.find(to);
        if (it != nodes_.end()) target = it->second;
    }
    if (target) {
        target->HandleRequestVote(args);
    }
}

void RaftCluster::SendAppendEntries(node_id_t from, node_id_t to, const AppendEntriesArgs& args) {
    std::shared_ptr<RaftNode> target = nullptr;
    {
        std::lock_guard<std::mutex> lock(cluster_mutex_);
        if (!CanCommunicate(from, to)) return;
        auto it = nodes_.find(to);
        if (it != nodes_.end()) target = it->second;
    }
    if (target) {
        target->HandleAppendEntries(args);
    }
}

void RaftCluster::SendInstallSnapshot(node_id_t from, node_id_t to, const InstallSnapshotArgs& args) {
    std::shared_ptr<RaftNode> target = nullptr;
    {
        std::lock_guard<std::mutex> lock(cluster_mutex_);
        if (!CanCommunicate(from, to)) return;
        auto it = nodes_.find(to);
        if (it != nodes_.end()) target = it->second;
    }
    if (target) {
        target->HandleInstallSnapshot(args);
    }
}

void RaftCluster::TickAll() {
    std::vector<std::shared_ptr<RaftNode>> current_nodes;
    {
        std::lock_guard<std::mutex> lock(cluster_mutex_);
        for (const auto& [id, n] : nodes_) {
            current_nodes.push_back(n);
        }
    }
    for (auto& n : current_nodes) {
        n->HandleTick();
    }
}

} // namespace raft
