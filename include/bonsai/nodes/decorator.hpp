#pragma once
#include "../structure/node.hpp"
#include <functional>
#include <chrono>
#include <memory>

namespace bonsai {

class Decorator : public Node {
public:
    using Func = std::function<Status(Status)>;
    
    Decorator(Func func, NodePtr child) 
        : func_(std::move(func)), child_(std::move(child)) {}
    
    inline Status tick(Blackboard& blackboard) override {
        if (halted_) return Status::Failure;
        Status childStatus = child_->tick(blackboard);
        return func_(childStatus);
    }
    
    inline void reset() override {
        halted_ = false;
        child_->reset();
    }
    
    inline void halt() override {
        halted_ = true;
        child_->halt();
    }

private:
    Func func_;
    NodePtr child_;
};

// Common decorator factories
namespace decorators {
    inline std::function<Status(Status)> Inverter() {
        return [](Status status) {
            if (status == Status::Success) return Status::Failure;
            if (status == Status::Failure) return Status::Success;
            return status;
        };
    }
    
    inline std::function<Status(Status)> Succeeder() {
        return [](Status status) {
            return (status != Status::Running) ? Status::Success : Status::Running;
        };
    }
    
    inline std::function<Status(Status)> Failer() {
        return [](Status status) {
            return (status != Status::Running) ? Status::Failure : Status::Running;
        };
    }
    
    inline std::function<Status(Status)> Repeat(int maxTimes = -1) {
        auto attempts = std::make_shared<int>(0);
        return [maxTimes, attempts](Status status) mutable -> Status {
            if (status == Status::Running) {
                return Status::Running;
            }
            
            if (status == Status::Success) {
                *attempts = 0; // Reset on success
                return Status::Success;
            }
            
            // On failure, try again if we have attempts left
            if (status == Status::Failure) {
                (*attempts)++;
                if (maxTimes > 0 && *attempts >= maxTimes) {
                    *attempts = 0; // Reset for next use
                    return Status::Failure;
                }
                return Status::Running; // Try again
            }
            
            return status;
        };
    }
    
    inline std::function<Status(Status)> Timeout(float seconds) {
        auto startTime = std::make_shared<std::chrono::steady_clock::time_point>();
        return [seconds, startTime](Status status) mutable -> Status {
            auto now = std::chrono::steady_clock::now();
            
            if (*startTime == std::chrono::steady_clock::time_point{}) {
                *startTime = now; // Initialize start time
            }
            
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - *startTime).count() / 1000.0f;
            
            if (elapsed >= seconds) {
                *startTime = {}; // Reset for next use
                return Status::Failure; // Timeout reached
            }
            
            if (status != Status::Running) {
                *startTime = {}; // Reset on completion
            }
            
            return status;
        };
    }
    
    inline std::function<Status(Status)> Cooldown(float seconds) {
        auto lastSuccess = std::make_shared<std::chrono::steady_clock::time_point>();
        return [seconds, lastSuccess](Status status) mutable -> Status {
            auto now = std::chrono::steady_clock::now();
            
            if (status == Status::Success) {
                *lastSuccess = now;
                return Status::Success;
            }
            
            if (*lastSuccess != std::chrono::steady_clock::time_point{}) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - *lastSuccess).count() / 1000.0f;
                if (elapsed < seconds) {
                    return Status::Failure; // Still in cooldown
                }
            }
            
            return status;
        };
    }
}

} // namespace bonsai