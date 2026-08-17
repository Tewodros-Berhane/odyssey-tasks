#include "raft_cluster.hpp"
#include <iostream>

int main() {
    std::cout << "Starting Distributed Raft Consensus Engine..." << std::endl;
    raft::RaftCluster cluster;
    auto s1 = std::make_shared<raft::MemoryStorage>();
    auto n1 = std::make_shared<raft::RaftNode>(1, std::vector<raft::node_id_t>{2, 3}, s1, &cluster);
    cluster.AddNode(n1);

    std::cout << "Cluster initialized with 1 node." << std::endl;
    return 0;
}
