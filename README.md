
# Bonsai - Behavior Tree Library

A lightweight, header-only C++20 behavior tree library designed for AI decision-making systems.

## Features

- **Header-only**: Simple integration, just include the main header
- **Modern C++20**: Takes advantage of concepts, ranges, and other modern features
- **Thread-safe**: Blackboard provides safe concurrent access to shared data
- **Extensible**: Easy to add custom node types and behaviors
- **Fluent API**: Builder pattern for intuitive tree construction

## 🚀 Quick Start

**New to behavior trees?** Start with our interactive tutorial:

```bash
# Build and run the comprehensive tutorial
cmake -DBONSAI_BUILD_EXAMPLES=ON . -B build
cmake --build build
./build/getting_started_tutorial
```

This tutorial walks you through every concept with practical examples and clear explanations!

## Complete API Tutorial

This comprehensive tutorial demonstrates every feature of the Bonsai library with detailed comments.

### Basic Setup and Core Concepts

```cpp
#include <bonsai/bonsai.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include <string>

using namespace bonsai;

int main() {
    std::cout << "=== Bonsai Behavior Tree Library Tutorial ===" << std::endl;
    
    // ========================================
    // 1. BLACKBOARD - Thread-safe shared memory
    // ========================================
    
    Blackboard blackboard;
    
    // Store different types of data
    blackboard.set("player_health", 100);
    blackboard.set("player_name", std::string("Hero"));
    blackboard.set("is_combat", false);
    blackboard.set("enemy_count", 3);
    blackboard.set("player_position", std::vector<double>{10.5, 0.0, 5.2});
    
    // Retrieve data safely with std::optional
    auto health = blackboard.get<int>("player_health");
    auto name = blackboard.get<std::string>("player_name");
    auto position = blackboard.get<std::vector<double>>("player_position");
    
    if (health.has_value()) {
        std::cout << "Player health: " << health.value() << std::endl;
    }
    
    if (name.has_value()) {
        std::cout << "Player name: " << name.value() << std::endl;
    }
    
    // Check if key exists
    if (blackboard.has("is_combat")) {
        std::cout << "Combat status is tracked" << std::endl;
    }
    
    // ========================================
    // 2. ACTION NODES - Basic building blocks
    // ========================================
    
    std::cout << "\n=== Action Nodes ===" << std::endl;
    
    // Simple action that always succeeds
    auto simple_action = std::make_unique<Action>([](Blackboard& bb) -> Status {
        std::cout << "Executing simple action" << std::endl;
        return Status::Success;
    });
    
    // Action that modifies blackboard
    auto modify_action = std::make_unique<Action>([](Blackboard& bb) -> Status {
        std::cout << "Modifying player health" << std::endl;
        auto current_health = bb.get<int>("player_health");
        if (current_health.has_value()) {
            bb.set("player_health", current_health.value() - 10);
        }
        return Status::Success;
    });
    
    // Action that can fail based on conditions
    auto conditional_action = std::make_unique<Action>([](Blackboard& bb) -> Status {
        auto health = bb.get<int>("player_health");
        if (health.has_value() && health.value() > 50) {
            std::cout << "Player is healthy, action succeeds" << std::endl;
            return Status::Success;
        } else {
            std::cout << "Player is low health, action fails" << std::endl;
            return Status::Failure;
        }
    });
    
    // Action that simulates running state
    static int counter = 0;
    auto running_action = std::make_unique<Action>([](Blackboard& bb) -> Status {
        counter++;
        std::cout << "Long running action step " << counter << std::endl;
        if (counter < 3) {
            return Status::Running;
        } else {
            counter = 0; // Reset for next time
            std::cout << "Long running action completed" << std::endl;
            return Status::Success;
        }
    });
    
    // Test individual actions
    std::cout << "Testing action nodes:" << std::endl;
    std::cout << "Simple action result: " << (int)simple_action->tick(blackboard) << std::endl;
    std::cout << "Modify action result: " << (int)modify_action->tick(blackboard) << std::endl;
    std::cout << "Conditional action result: " << (int)conditional_action->tick(blackboard) << std::endl;
    
    // ========================================
    // 3. SEQUENCE NODES - Execute in order until failure
    // ========================================
    
    std::cout << "\n=== Sequence Nodes ===" << std::endl;
    
    auto sequence_tree = Builder()
        .sequence()
            .action([](Blackboard& bb) -> Status {
                std::cout << "Sequence step 1: Check prerequisites" << std::endl;
                return Status::Success;
            })
            .action([](Blackboard& bb) -> Status {
                std::cout << "Sequence step 2: Prepare resources" << std::endl;
                bb.set("resources_ready", true);
                return Status::Success;
            })
            .action([](Blackboard& bb) -> Status {
                std::cout << "Sequence step 3: Execute main task" << std::endl;
                auto ready = bb.get<bool>("resources_ready");
                if (ready.has_value() && ready.value()) {
                    std::cout << "Task executed successfully!" << std::endl;
                    return Status::Success;
                }
                return Status::Failure;
            })
        .end()
        .build();
    
    std::cout << "Executing sequence (all should succeed):" << std::endl;
    Status seq_result = sequence_tree.tick();
    std::cout << "Sequence result: " << (seq_result == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;
    
    // Sequence that fails on second step
    auto failing_sequence = Builder()
        .sequence()
            .action([](Blackboard& bb) -> Status {
                std::cout << "Step 1: Success" << std::endl;
                return Status::Success;
            })
            .action([](Blackboard& bb) -> Status {
                std::cout << "Step 2: Failure (stops here)" << std::endl;
                return Status::Failure;
            })
            .action([](Blackboard& bb) -> Status {
                std::cout << "Step 3: Should not execute" << std::endl;
                return Status::Success;
            })
        .end()
        .build();
    
    std::cout << "\nExecuting failing sequence:" << std::endl;
    Status fail_result = failing_sequence.tick();
    std::cout << "Failing sequence result: " << (fail_result == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;
    
    // ========================================
    // 4. SELECTOR NODES - Execute until success
    // ========================================
    
    std::cout << "\n=== Selector Nodes ===" << std::endl;
    
    auto selector_tree = Builder()
        .selector()
            .action([](Blackboard& bb) -> Status {
                std::cout << "Option 1: Check if enemy is weak" << std::endl;
                auto enemy_count = bb.get<int>("enemy_count");
                if (enemy_count.has_value() && enemy_count.value() <= 1) {
                    std::cout << "Enemy is weak, attack directly!" << std::endl;
                    return Status::Success;
                }
                return Status::Failure;
            })
            .action([](Blackboard& bb) -> Status {
                std::cout << "Option 2: Check if we have ranged weapon" << std::endl;
                // Simulate having ranged weapon
                bb.set("has_ranged_weapon", true);
                auto has_weapon = bb.get<bool>("has_ranged_weapon");
                if (has_weapon.has_value() && has_weapon.value()) {
                    std::cout << "Use ranged attack!" << std::endl;
                    return Status::Success;
                }
                return Status::Failure;
            })
            .action([](Blackboard& bb) -> Status {
                std::cout << "Option 3: Retreat (fallback option)" << std::endl;
                return Status::Success; // Always succeeds as last resort
            })
        .end()
        .build();
    
    std::cout << "Executing selector (should succeed on option 2):" << std::endl;
    Status sel_result = selector_tree.tick();
    std::cout << "Selector result: " << (sel_result == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;
    
    // ========================================
    // 5. PARALLEL NODES - Execute children simultaneously
    // ========================================
    
    std::cout << "\n=== Parallel Nodes ===" << std::endl;
    
    // Parallel node that succeeds when 2 out of 3 children succeed
    auto parallel_tree = Builder()
        .parallel(2) // Success threshold = 2
            .action([](Blackboard& bb) -> Status {
                std::cout << "Parallel task 1: Scanning for enemies" << std::endl;
                return Status::Success;
            })
            .action([](Blackboard& bb) -> Status {
                std::cout << "Parallel task 2: Monitoring health" << std::endl;
                return Status::Success;
            })
            .action([](Blackboard& bb) -> Status {
                std::cout << "Parallel task 3: Communication (fails)" << std::endl;
                return Status::Failure;
            })
        .end()
        .build();
    
    std::cout << "Executing parallel (2/3 success threshold):" << std::endl;
    Status par_result = parallel_tree.tick();
    std::cout << "Parallel result: " << (par_result == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;
    
    // ========================================
    // 6. DECORATOR NODES - Modify child behavior
    // ========================================
    
    std::cout << "\n=== Decorator Nodes ===" << std::endl;
    
    // Inverter - Flips success/failure
    auto inverter_tree = Builder()
        .inverter()
            .action([](Blackboard& bb) -> Status {
                std::cout << "Action that fails" << std::endl;
                return Status::Failure; // Will be inverted to Success
            })
        .end()
        .build();
    
    std::cout << "Executing inverter (failure becomes success):" << std::endl;
    Status inv_result = inverter_tree.tick();
    std::cout << "Inverter result: " << (inv_result == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;
    
    // Succeeder - Always returns success
    auto succeeder_tree = Builder()
        .succeeder()
            .action([](Blackboard& bb) -> Status {
                std::cout << "Action that fails but succeeder overrides" << std::endl;
                return Status::Failure; // Will become Success
            })
        .end()
        .build();
    
    std::cout << "\nExecuting succeeder:" << std::endl;
    Status succ_result = succeeder_tree.tick();
    std::cout << "Succeeder result: " << (succ_result == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;
    
    // Failer - Always returns failure
    auto failer_tree = Builder()
        .failer()
            .action([](Blackboard& bb) -> Status {
                std::cout << "Action that succeeds but failer overrides" << std::endl;
                return Status::Success; // Will become Failure
            })
        .end()
        .build();
    
    std::cout << "\nExecuting failer:" << std::endl;
    Status fail_dec_result = failer_tree.tick();
    std::cout << "Failer result: " << (fail_dec_result == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;
    
    // Repeater - Executes child multiple times
    int repeat_count = 0;
    auto repeat_tree = Builder()
        .repeat(3)
            .action([&repeat_count](Blackboard& bb) -> Status {
                repeat_count++;
                std::cout << "Repeat execution #" << repeat_count << std::endl;
                return Status::Success;
            })
        .end()
        .build();
    
    std::cout << "\nExecuting repeater (3 times):" << std::endl;
    Status rep_result = repeat_tree.tick();
    std::cout << "Repeat result: " << (rep_result == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;
    
    // Timeout - Limits execution time
    auto timeout_tree = Builder()
        .timeout(1.0) // 1 second timeout
            .action([](Blackboard& bb) -> Status {
                std::cout << "Quick action (completes before timeout)" << std::endl;
                return Status::Success;
            })
        .end()
        .build();
    
    std::cout << "\nExecuting timeout:" << std::endl;
    Status timeout_result = timeout_tree.tick();
    std::cout << "Timeout result: " << (timeout_result == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;
    
    // Cooldown - Prevents rapid re-execution
    auto cooldown_tree = Builder()
        .cooldown(0.5) // 0.5 second cooldown
            .action([](Blackboard& bb) -> Status {
                std::cout << "Action with cooldown" << std::endl;
                return Status::Success;
            })
        .end()
        .build();
    
    std::cout << "\nExecuting cooldown (first time):" << std::endl;
    Status cool_result1 = cooldown_tree.tick();
    std::cout << "Cooldown result 1: " << (cool_result1 == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;
    
    std::cout << "Executing cooldown (immediate retry - should fail):" << std::endl;
    Status cool_result2 = cooldown_tree.tick();
    std::cout << "Cooldown result 2: " << (cool_result2 == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;
    
    // ========================================
    // 7. UTILITY-BASED SELECTION - AI Decision Making
    // ========================================
    
    std::cout << "\n=== Utility-Based Selection ===" << std::endl;
    
    auto utility_tree = Builder()
        .utilitySelector()
            .action([](Blackboard& bb) -> Status {
                // Calculate utility for attacking
                auto health = bb.get<int>("player_health");
                auto enemy_count = bb.get<int>("enemy_count");
                
                double utility = 0.0;
                if (health.has_value() && enemy_count.has_value()) {
                    // Higher utility when health is high and enemies are few
                    utility = (health.value() / 100.0) * (1.0 / enemy_count.value());
                }
                
                bb.set("utility_score", utility);
                std::cout << "Attack option utility: " << utility << std::endl;
                return Status::Success;
            })
            .action([](Blackboard& bb) -> Status {
                // Calculate utility for defending
                auto health = bb.get<int>("player_health");
                auto enemy_count = bb.get<int>("enemy_count");
                
                double utility = 0.0;
                if (health.has_value() && enemy_count.has_value()) {
                    // Higher utility when health is low or enemies are many
                    utility = (1.0 - health.value() / 100.0) + (enemy_count.value() / 10.0);
                }
                
                bb.set("utility_score", utility);
                std::cout << "Defend option utility: " << utility << std::endl;
                return Status::Success;
            })
            .action([](Blackboard& bb) -> Status {
                // Calculate utility for fleeing
                auto health = bb.get<int>("player_health");
                auto enemy_count = bb.get<int>("enemy_count");
                
                double utility = 0.0;
                if (health.has_value() && enemy_count.has_value()) {
                    // Higher utility when health is very low
                    utility = (health.value() < 30) ? 0.9 : 0.1;
                }
                
                bb.set("utility_score", utility);
                std::cout << "Flee option utility: " << utility << std::endl;
                return Status::Success;
            })
        .end()
        .build();
    
    std::cout << "Executing utility selector:" << std::endl;
    Status util_result = utility_tree.tick();
    std::cout << "Utility selector result: " << (util_result == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;
    
    // ========================================
    // 8. WEIGHTED RANDOM SELECTION
    // ========================================
    
    std::cout << "\n=== Weighted Random Selection ===" << std::endl;
    
    auto weighted_tree = Builder()
        .weightedRandomSelector()
            .action([](Blackboard& bb) -> Status {
                bb.set("weight", 3.0); // High weight - more likely
                std::cout << "Common action (weight: 3.0)" << std::endl;
                return Status::Success;
            })
            .action([](Blackboard& bb) -> Status {
                bb.set("weight", 1.0); // Medium weight
                std::cout << "Uncommon action (weight: 1.0)" << std::endl;
                return Status::Success;
            })
            .action([](Blackboard& bb) -> Status {
                bb.set("weight", 0.5); // Low weight - less likely
                std::cout << "Rare action (weight: 0.5)" << std::endl;
                return Status::Success;
            })
        .end()
        .build();
    
    std::cout << "Executing weighted random selector (3 times):" << std::endl;
    for (int i = 0; i < 3; ++i) {
        Status weight_result = weighted_tree.tick();
        std::cout << "  Weighted result " << (i+1) << ": " << (weight_result == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;
    }
    
    // ========================================
    // 9. COMPLEX NESTED TREES
    // ========================================
    
    std::cout << "\n=== Complex Nested Trees ===" << std::endl;
    
    // Realistic AI behavior tree for a game character
    auto game_ai_tree = Builder()
        .selector() // Try different strategies
            // Strategy 1: Aggressive (when health is high)
            .sequence()
                .action([](Blackboard& bb) -> Status {
                    auto health = bb.get<int>("player_health");
                    if (health.has_value() && health.value() > 70) {
                        std::cout << "Health is high, being aggressive" << std::endl;
                        return Status::Success;
                    }
                    return Status::Failure;
                })
                .selector() // Choose attack type
                    .sequence() // Melee attack
                        .action([](Blackboard& bb) -> Status {
                            std::cout << "Check if enemy is close" << std::endl;
                            bb.set("enemy_distance", 2.0);
                            auto distance = bb.get<double>("enemy_distance");
                            return (distance.has_value() && distance.value() < 3.0) ? Status::Success : Status::Failure;
                        })
                        .action([](Blackboard& bb) -> Status {
                            std::cout << "Executing melee attack!" << std::endl;
                            return Status::Success;
                        })
                    .end()
                    .sequence() // Ranged attack
                        .action([](Blackboard& bb) -> Status {
                            std::cout << "Check if we have ranged weapon" << std::endl;
                            return Status::Success; // Assume we have it
                        })
                        .action([](Blackboard& bb) -> Status {
                            std::cout << "Executing ranged attack!" << std::endl;
                            return Status::Success;
                        })
                    .end()
                .end()
            .end()
            
            // Strategy 2: Defensive (when health is medium)
            .sequence()
                .action([](Blackboard& bb) -> Status {
                    auto health = bb.get<int>("player_health");
                    if (health.has_value() && health.value() >= 30 && health.value() <= 70) {
                        std::cout << "Health is medium, being defensive" << std::endl;
                        return Status::Success;
                    }
                    return Status::Failure;
                })
                .parallel(1) // Do defensive actions simultaneously
                    .action([](Blackboard& bb) -> Status {
                        std::cout << "Raising shield" << std::endl;
                        return Status::Success;
                    })
                    .action([](Blackboard& bb) -> Status {
                        std::cout << "Looking for cover" << std::endl;
                        return Status::Success;
                    })
                .end()
            .end()
            
            // Strategy 3: Retreat (when health is low)
            .sequence()
                .action([](Blackboard& bb) -> Status {
                    auto health = bb.get<int>("player_health");
                    if (health.has_value() && health.value() < 30) {
                        std::cout << "Health is low, retreating!" << std::endl;
                        return Status::Success;
                    }
                    return Status::Failure;
                })
                .action([](Blackboard& bb) -> Status {
                    std::cout << "Finding escape route" << std::endl;
                    return Status::Success;
                })
                .action([](Blackboard& bb) -> Status {
                    std::cout << "Running away!" << std::endl;
                    return Status::Success;
                })
            .end()
        .end()
        .build();
    
    // Test with different health values
    std::vector<int> health_values = {90, 50, 20};
    for (int health : health_values) {
        std::cout << "\nTesting AI with health: " << health << std::endl;
        game_ai_tree.blackboard().set("player_health", health);
        Status ai_result = game_ai_tree.tick();
        std::cout << "AI decision result: " << (ai_result == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;
    }
    
    // ========================================
    // 10. TREE EXECUTION AND BLACKBOARD ACCESS
    // ========================================
    
    std::cout << "\n=== Tree Execution and Blackboard Access ===" << std::endl;
    
    auto tree_with_blackboard = Builder()
        .sequence()
            .action([](Blackboard& bb) -> Status {
                bb.set("step", 1);
                bb.set("message", std::string("First step completed"));
                return Status::Success;
            })
            .action([](Blackboard& bb) -> Status {
                auto step = bb.get<int>("step");
                if (step.has_value()) {
                    bb.set("step", step.value() + 1);
                }
                bb.set("message", std::string("Second step completed"));
                return Status::Success;
            })
            .action([](Blackboard& bb) -> Status {
                auto step = bb.get<int>("step");
                auto message = bb.get<std::string>("message");
                
                std::cout << "Final step: " << (step.has_value() ? step.value() : 0) << std::endl;
                std::cout << "Final message: " << (message.has_value() ? message.value() : "none") << std::endl;
                
                return Status::Success;
            })
        .end()
        .build();
    
    std::cout << "Executing tree with blackboard tracking:" << std::endl;
    Status bb_result = tree_with_blackboard.tick();
    std::cout << "Blackboard tree result: " << (bb_result == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;
    
    // Access tree's internal blackboard after execution
    auto& tree_bb = tree_with_blackboard.blackboard();
    auto final_step = tree_bb.get<int>("step");
    if (final_step.has_value()) {
        std::cout << "Final step from tree blackboard: " << final_step.value() << std::endl;
    }
    
    // ========================================
    // 11. ADVANCED PATTERNS AND TECHNIQUES
    // ========================================
    
    std::cout << "\n=== Advanced Patterns ===" << std::endl;
    
    // State machine using selector + sequences
    auto state_machine = Builder()
        .selector()
            // Idle state
            .sequence()
                .action([](Blackboard& bb) -> Status {
                    auto state = bb.get<std::string>("current_state");
                    if (!state.has_value() || state.value() == "idle") {
                        bb.set("current_state", std::string("idle"));
                        std::cout << "State: IDLE - Waiting for input" << std::endl;
                        
                        // Simulate input
                        bb.set("input_received", true);
                        return Status::Success;
                    }
                    return Status::Failure;
                })
                .action([](Blackboard& bb) -> Status {
                    auto input = bb.get<bool>("input_received");
                    if (input.has_value() && input.value()) {
                        bb.set("current_state", std::string("processing"));
                        return Status::Success;
                    }
                    return Status::Failure;
                })
            .end()
            
            // Processing state
            .sequence()
                .action([](Blackboard& bb) -> Status {
                    auto state = bb.get<std::string>("current_state");
                    if (state.has_value() && state.value() == "processing") {
                        std::cout << "State: PROCESSING - Handling input" << std::endl;
                        return Status::Success;
                    }
                    return Status::Failure;
                })
                .action([](Blackboard& bb) -> Status {
                    // Simulate processing
                    std::cout << "Processing complete" << std::endl;
                    bb.set("current_state", std::string("idle"));
                    bb.set("input_received", false);
                    return Status::Success;
                })
            .end()
        .end()
        .build();
    
    std::cout << "Executing state machine:" << std::endl;
    for (int i = 0; i < 3; ++i) {
        std::cout << "Cycle " << (i+1) << ":" << std::endl;
        Status sm_result = state_machine.tick();
        std::cout << "State machine result: " << (sm_result == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;
    }
    
    std::cout << "\n=== Tutorial Complete ===" << std::endl;
    std::cout << "This tutorial demonstrated every major feature of the Bonsai library:" << std::endl;
    std::cout << "• Blackboard for shared data storage" << std::endl;
    std::cout << "• Action nodes for basic behaviors" << std::endl;
    std::cout << "• Sequence, Selector, and Parallel composite nodes" << std::endl;
    std::cout << "• Decorator nodes (Inverter, Succeeder, Failer, Repeat, Timeout, Cooldown)" << std::endl;
    std::cout << "• Utility-based and weighted random selection" << std::endl;
    std::cout << "• Complex nested tree structures" << std::endl;
    std::cout << "• Advanced patterns like state machines" << std::endl;
    
    return 0;
}
```

## Quick Start Guide

For those who want to get started quickly, here's a minimal example:

```cpp
#include <bonsai/bonsai.hpp>

using namespace bonsai;

int main() {
    // Create a simple behavior tree
    auto tree = Builder()
        .sequence()
            .action([](Blackboard& bb) {
                std::cout << "Starting task..." << std::endl;
                return Status::Success;
            })
            .action([](Blackboard& bb) {
                std::cout << "Task completed!" << std::endl;
                return Status::Success;
            })
        .end()
        .build();
    
    // Execute the tree
    Status result = tree.tick();
    
    return 0;
}
```

## Core Concepts Reference

### Status Values
- `Status::Success` - Node completed successfully
- `Status::Failure` - Node failed to complete  
- `Status::Running` - Node is still executing (for async operations)

### Node Types

#### Composite Nodes
- **Sequence**: Executes children in order, succeeds when all succeed
- **Selector**: Tries children in order, succeeds when one succeeds  
- **Parallel**: Executes all children simultaneously

#### Decorator Nodes
- **Inverter**: Flips Success ↔ Failure
- **Repeater**: Retries child on failure
- **Timeout**: Fails child after time limit
- **Cooldown**: Prevents execution during cooldown period

#### Leaf Nodes
- **Action**: Executes custom logic

### Blackboard

Thread-safe shared memory system for communication between nodes:

```cpp
// Store data of any type
tree.blackboard().set("health", 100);
tree.blackboard().set("name", std::string("Player"));
tree.blackboard().set("position", std::vector<double>{1.0, 2.0, 3.0});

// Retrieve data safely with std::optional
auto health = tree.blackboard().get<int>("health");
if (health.has_value() && health.value() < 50) {
    // Low health logic
}
```

## Advanced Features

### Utility-based Selection

Create AI that adapts to changing conditions by scoring options:

```cpp
auto tree = Builder()
    .utilitySelector()
        .action([](Blackboard& bb) -> Status {
            // Calculate utility for this action
            auto hunger = bb.get<float>("hunger").value_or(0.0f);
            bb.set("utility_score", hunger);
            
            if (hunger > 0.5f) {
                std::cout << "Eating!" << std::endl;
                return Status::Success;
            }
            return Status::Failure;
        })
        .action([](Blackboard& bb) -> Status {
            // Calculate utility for this action
            auto tiredness = bb.get<float>("tiredness").value_or(0.0f);
            bb.set("utility_score", tiredness);
            
            if (tiredness > 0.5f) {
                std::cout << "Sleeping!" << std::endl;
                return Status::Success;
            }
            return Status::Failure;
        })
    .end()
    .build();
```

### Weighted Random Selection

Introduce controlled randomness for more varied behaviors:

```cpp
auto tree = Builder()
    .weightedRandomSelector()
        .action([](Blackboard& bb) -> Status {
            bb.set("weight", 3.0); // High probability
            std::cout << "Common action" << std::endl;
            return Status::Success;
        })
        .action([](Blackboard& bb) -> Status {
            bb.set("weight", 1.0); // Lower probability
            std::cout << "Rare action" << std::endl;
            return Status::Success;
        })
    .end()
    .build();
```

## Example Projects

The `examples/` directory contains comprehensive demonstrations:

- **`getting_started_tutorial.cpp`** - 🌟 **START HERE!** Interactive tutorial with step-by-step explanations
- **`main.cpp`** - Robot navigation with PID controllers
- **`utility_demo.cpp`** - AI decision making with utility functions  
- **`builder_utility_demo.cpp`** - Advanced builder patterns
- **`comprehensive_test.cpp`** - Full feature test suite
- **`pea_harvester_demo.cpp`** - Complex real-world optimization scenario

## Building

This is a header-only library. Simply include the main header:

```cpp
#include <bonsai/bonsai.hpp>
```

### Building Examples

```bash
mkdir build && cd build
cmake -DBONSAI_BUILD_EXAMPLES=ON ..
make

# Run examples
./main
./utility_demo
./comprehensive_test
./pea_harvester_demo
```

## Requirements

- C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 2019+)
- CMake 3.15+ (for building examples)
- Standard library with threading support

## License

MIT License - see LICENSE file for details.
