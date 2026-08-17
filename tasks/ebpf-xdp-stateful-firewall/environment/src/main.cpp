#include "firewall_engine.hpp"
#include <iostream>

int main() {
    std::cout << "Starting eBPF/XDP Stateful Firewall Control Plane..." << std::endl;
    firewall::FirewallEngine engine;
    std::cout << "Firewall Engine initialized." << std::endl;
    return 0;
}
