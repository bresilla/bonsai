#include <bonsai/bonsai.hpp>
#include <iostream>

using namespace bonsai;

int main() {
    std::cout << "Starting minimal test..." << std::endl;
    
    // Create a simple action
    auto tree = Builder()
        .action([](Blackboard& bb) -> Status {
            std::cout << "Action executed" << std::endl;
            return Status::Success;
        })
        .build();
    
    std::cout << "Tree built successfully" << std::endl;
    
    // Tick the tree
    auto status = tree.tick();
    std::cout << "Tree ticked, status: " << (int)status << std::endl;
    
    return 0;
}
