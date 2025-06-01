#pragma once
#include "../structure/node.hpp"
#include <vector>

namespace bonsai {

class Sequence : public Node {
public:
    inline void addChild(const NodePtr& child) { 
        children_.emplace_back(child); 
    }
    
    inline Status tick(Blackboard& blackboard) override {
        if (halted_) return Status::Failure;
        
        while (currentIndex_ < children_.size()) {
            Status status = children_[currentIndex_]->tick(blackboard);
            if (status == Status::Running) return Status::Running;
            if (status == Status::Failure) { 
                reset(); 
                return Status::Failure; 
            }
            ++currentIndex_;
        }
        reset();
        return Status::Success;
    }
    
    inline void reset() override {
        currentIndex_ = 0;
        halted_ = false;
        for (auto& child : children_) {
            child->reset();
        }
    }
    
    inline void halt() override {
        halted_ = true;
        for (auto& child : children_) {
            child->halt();
        }
    }

private:
    std::vector<NodePtr> children_;
    size_t currentIndex_ = 0;
};

} // namespace bonsai