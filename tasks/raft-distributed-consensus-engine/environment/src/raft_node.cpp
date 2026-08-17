#include "raft_node.hpp"
#include "raft_cluster.hpp"
#include <iostream>

namespace raft {

RaftNode::RaftNode(node_id_t id, std::vector<node_id_t> peers, std::shared_ptr<Storage> storage, RaftCluster* cluster)
    : id_(id), peers_(peers), storage_(storage), cluster_(cluster) {}

term_t RaftNode::GetTerm() const {
    return storage_->GetCurrentTerm();
}

void RaftNode::StepDown(term_t new_term) {
    storage_->SaveHardState(new_term, std::nullopt);
    role_ = Role::FOLLOWER;
    election_elapsed_ = 0;
}

void RaftNode::HandleTick() {
    std::lock_guard<std::mutex> lock(node_mutex_);
    if (role_ == Role::LEADER) {
        heartbeat_elapsed_++;
        if (heartbeat_elapsed_ >= heartbeat_timeout_) {
            heartbeat_elapsed_ = 0;
            BroadcastHeartbeats();
        }
    } else {
        election_elapsed_++;
        if (election_elapsed_ >= randomized_election_timeout_) {
            election_elapsed_ = 0;
            StartElection();
        }
    }
}

void RaftNode::StartElection() {
    term_t term = storage_->GetCurrentTerm() + 1;
    storage_->SaveHardState(term, id_);
    role_ = Role::CANDIDATE;
    votes_granted_.clear();
    votes_granted_.insert(id_);

    RequestVoteArgs args;
    args.term = term;
    args.candidate_id = id_;
    args.last_log_index = storage_->GetLastIndex();
    args.last_log_term = storage_->GetLastTerm();
    args.is_pre_vote = false;

    for (node_id_t peer : peers_) {
        if (cluster_) {
            cluster_->SendRequestVote(id_, peer, args);
        }
    }
}

RequestVoteReply RaftNode::HandleRequestVote(const RequestVoteArgs& args) {
    std::lock_guard<std::mutex> lock(node_mutex_);
    RequestVoteReply reply;
    term_t cur_term = storage_->GetCurrentTerm();

    if (args.term < cur_term) {
        reply.term = cur_term;
        reply.vote_granted = false;
        return reply;
    }

    if (args.term > cur_term) {
        StepDown(args.term);
    }

    auto voted_for = storage_->GetVotedFor();
    bool can_vote = (!voted_for.has_value() || *voted_for == args.candidate_id);
    index_t last_idx = storage_->GetLastIndex();
    term_t last_term = storage_->GetLastTerm();
    bool log_ok = (args.last_log_term > last_term) ||
                  (args.last_log_term == last_term && args.last_log_index >= last_idx);

    if (can_vote && log_ok) {
        storage_->SaveHardState(args.term, args.candidate_id);
        reply.vote_granted = true;
        election_elapsed_ = 0;
    } else {
        reply.vote_granted = false;
    }

    reply.term = storage_->GetCurrentTerm();
    return reply;
}

AppendEntriesReply RaftNode::HandleAppendEntries(const AppendEntriesArgs& args) {
    std::lock_guard<std::mutex> lock(node_mutex_);
    AppendEntriesReply reply;
    term_t cur_term = storage_->GetCurrentTerm();

    if (args.term < cur_term) {
        reply.term = cur_term;
        reply.success = false;
        return reply;
    }

    if (args.term > cur_term || role_ == Role::CANDIDATE) {
        StepDown(args.term);
    }
    election_elapsed_ = 0;

    if (args.prev_log_index > storage_->GetLastIndex() ||
        storage_->GetTerm(args.prev_log_index) != args.prev_log_term) {
        reply.term = cur_term;
        reply.success = false;
        return reply;
    }

    if (!args.entries.empty()) {
        storage_->Truncate(args.prev_log_index + 1);
        storage_->Append(args.entries);
    }

    if (args.leader_commit > commit_index_) {
        commit_index_ = std::min(args.leader_commit, storage_->GetLastIndex());
    }

    reply.term = cur_term;
    reply.success = true;
    reply.match_index = storage_->GetLastIndex();
    return reply;
}

InstallSnapshotReply RaftNode::HandleInstallSnapshot(const InstallSnapshotArgs& args) {
    std::lock_guard<std::mutex> lock(node_mutex_);
    InstallSnapshotReply reply;
    term_t cur_term = storage_->GetCurrentTerm();
    if (args.term < cur_term) {
        reply.term = cur_term;
        return reply;
    }
    if (args.term > cur_term) {
        StepDown(args.term);
    }
    election_elapsed_ = 0;

    storage_->ApplySnapshot(args.last_included_index, args.last_included_term, args.data);
    commit_index_ = std::max(commit_index_, args.last_included_index);
    last_applied_ = std::max(last_applied_, args.last_included_index);

    reply.term = cur_term;
    return reply;
}

bool RaftNode::Propose(const std::string& data) {
    std::lock_guard<std::mutex> lock(node_mutex_);
    if (role_ != Role::LEADER) return false;

    LogEntry entry;
    entry.term = storage_->GetCurrentTerm();
    entry.index = storage_->GetLastIndex() + 1;
    entry.type = EntryType::NORMAL;
    entry.data = data;

    storage_->Append({entry});
    match_index_[id_] = entry.index;
    BroadcastAppendEntries();
    return true;
}

void RaftNode::BroadcastHeartbeats() {
    AppendEntriesArgs args;
    args.term = storage_->GetCurrentTerm();
    args.leader_id = id_;
    args.leader_commit = commit_index_;
    args.prev_log_index = storage_->GetLastIndex();
    args.prev_log_term = storage_->GetLastTerm();

    for (node_id_t peer : peers_) {
        if (cluster_) {
            cluster_->SendAppendEntries(id_, peer, args);
        }
    }
}

void RaftNode::BroadcastAppendEntries() {
    BroadcastHeartbeats();
}

} // namespace raft
