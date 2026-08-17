#include "raft_cluster.hpp"
#include <cassert>
#include <iostream>

void TestBasicElection() {
    raft::RaftCluster cluster;
    auto s1 = std::make_shared<raft::MemoryStorage>();
    auto s2 = std::make_shared<raft::MemoryStorage>();
    auto s3 = std::make_shared<raft::MemoryStorage>();

    auto n1 = std::make_shared<raft::RaftNode>(1, std::vector<raft::node_id_t>{2, 3}, s1, &cluster);
    auto n2 = std::make_shared<raft::RaftNode>(2, std::vector<raft::node_id_t>{1, 3}, s2, &cluster);
    auto n3 = std::make_shared<raft::RaftNode>(3, std::vector<raft::node_id_t>{1, 2}, s3, &cluster);

    cluster.AddNode(n1);
    cluster.AddNode(n2);
    cluster.AddNode(n3);

    for (int i = 0; i < 30; ++i) {
        cluster.TickAll();
    }

    std::cout << "TestBasicElection completed ticks without crashing." << std::endl;
}

int main() {
    TestBasicElection();
    return 0;
}
