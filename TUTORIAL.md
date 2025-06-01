# Bonsai Behavior Tree Tutorial

```
   ____                        _ 
  |  _ \                      (_)
  | |_) | ___  _ __  ___  __ _ _ 
  |  _ < / _ \| '_ \/ __|/ _` | |
  | |_) | (_) | | | \__ \ (_| | |
  |____/ \___/|_| |_|___/\__,_|_|
                                 
   Behavior Tree Library Tutorial
```

Welcome to the comprehensive Bonsai behavior tree tutorial! This guide will take you from beginner to advanced usage through practical examples and clear explanations.

## Table of Contents

1. [Basic Actions and Status](#lesson-1-basic-actions-and-status)
2. [The Blackboard - Shared Memory](#lesson-2-the-blackboard---shared-memory)
3. [Sequence Nodes - Do This, Then That](#lesson-3-sequence-nodes---do-this-then-that)
4. [Selector Nodes - Try This, Or That](#lesson-4-selector-nodes---try-this-or-that)
5. [Parallel Nodes - Do Multiple Things](#lesson-5-parallel-nodes---do-multiple-things)
6. [Decorators - Modify Behavior](#lesson-6-decorators---modify-behavior)
7. [Real-World Example - AI Guard](#lesson-7-real-world-example---ai-guard)
8. [Manual Node Creation](#lesson-8-manual-node-creation)

---

## Lesson 1: Basic Actions and Status

Actions are the building blocks of behavior trees. They return `Status::Success`, `Status::Failure`, or `Status::Running`.

```cpp
#include <bonsai/bonsai.hpp>
#include <iostream>

using namespace bonsai;

int main() {
    // Create simple actions with different outcomes
    auto always_succeed = [](Blackboard& bb) -> Status {
        std::cout << "✓ Action that always succeeds" << std::endl;
        return Status::Success;
    };
    
    auto always_fail = [](Blackboard& bb) -> Status {
        std::cout << "✗ Action that always fails" << std::endl;
        return Status::Failure;
    };
    
    auto running_action = [](Blackboard& bb) -> Status {
        // Get or initialize counter in blackboard
        auto counter_opt = bb.get<int>("action_counter");
        int counter = counter_opt.value_or(0);
        counter++;
        bb.set("action_counter", counter);
        
        std::cout << "⟳ Running action (step " << counter << "/3)" << std::endl;
        if (counter < 3) {
            return Status::Running;
        } else {
            bb.set("action_counter", 0); // Reset for next run
            std::cout << "✓ Running action completed!" << std::endl;
            return Status::Success;
        }
    };
    
    // Test individual actions
    Tree succeed_tree(std::make_unique<Action>(always_succeed));
    Tree fail_tree(std::make_unique<Action>(always_fail));
    Tree running_tree(std::make_unique<Action>(running_action));
    
    std::cout << "Testing individual actions:" << std::endl;
    std::cout << "Succeed result: " << (succeed_tree.tick() == Status::Success ? "SUCCESS" : "FAIL") << std::endl;
    std::cout << "Fail result: " << (fail_tree.tick() == Status::Success ? "SUCCESS" : "FAIL") << std::endl;
    
    // Running action needs multiple ticks
    std::cout << "Running action test:" << std::endl;
    Status running_result = Status::Running;
    while (running_result == Status::Running) {
        running_result = running_tree.tick();
    }
    std::cout << "Final result: " << (running_result == Status::Success ? "SUCCESS" : "FAIL") << std::endl;
    
    return 0;
}
```

**Key Concepts:**
- Actions are lambda functions that take a `Blackboard&` and return a `Status`
- `Status::Success` - The action completed successfully
- `Status::Failure` - The action failed
- `Status::Running` - The action is still in progress (needs more ticks)

---

## Lesson 2: The Blackboard - Shared Memory

The Blackboard is like a shared whiteboard where nodes can store and retrieve data to coordinate their behavior.

```cpp
auto blackboard_demo = Builder()
    .sequence()
        .action([](Blackboard& bb) -> Status {
            std::cout << "📝 Storing player data on blackboard..." << std::endl;
            bb.set("player_name", std::string("Alice"));
            bb.set("player_level", 42);
            bb.set("player_health", 85.5);
            bb.set("has_key", true);
            return Status::Success;
        })
        .action([](Blackboard& bb) -> Status {
            std::cout << "📖 Reading player data from blackboard..." << std::endl;
            
            auto name = bb.get<std::string>("player_name");
            auto level = bb.get<int>("player_level");
            auto health = bb.get<double>("player_health");
            auto has_key = bb.get<bool>("has_key");
            
            if (name.has_value()) {
                std::cout << "   Player: " << name.value() << std::endl;
            }
            if (level.has_value()) {
                std::cout << "   Level: " << level.value() << std::endl;
            }
            if (health.has_value()) {
                std::cout << "   Health: " << health.value() << "%" << std::endl;
            }
            if (has_key.has_value()) {
                std::cout << "   Has key: " << (has_key.value() ? "Yes" : "No") << std::endl;
            }
            
            return Status::Success;
        })
    .end()
    .build();

blackboard_demo.tick();
```

**Key Concepts:**
- The blackboard stores typed data by string keys
- Use `bb.set<T>(key, value)` to store data
- Use `bb.get<T>(key)` to retrieve data (returns `std::optional<T>`)
- The blackboard is thread-safe for concurrent access
- Data persists between tree ticks

---

## Lesson 3: Sequence Nodes - Do This, Then That

Sequences execute children in order. If any child fails, the sequence stops and returns failure.

```cpp
// Successful sequence
auto successful_sequence = Builder()
    .sequence()
        .action([](Blackboard& bb) -> Status {
            std::cout << "1️⃣ Check inventory" << std::endl;
            bb.set("has_materials", true);
            return Status::Success;
        })
        .action([](Blackboard& bb) -> Status {
            std::cout << "2️⃣ Craft item" << std::endl;
            auto has_materials = bb.get<bool>("has_materials");
            if (has_materials.value_or(false)) {
                bb.set("item_crafted", true);
                return Status::Success;
            }
            return Status::Failure;
        })
        .action([](Blackboard& bb) -> Status {
            std::cout << "3️⃣ Add to inventory" << std::endl;
            auto item_crafted = bb.get<bool>("item_crafted");
            if (item_crafted.value_or(false)) {
                std::cout << "✨ Item successfully crafted and stored!" << std::endl;
                return Status::Success;
            }
            return Status::Failure;
        })
    .end()
    .build();

std::cout << "Executing successful crafting sequence:" << std::endl;
Status craft_result = successful_sequence.tick();
std::cout << "Result: " << (craft_result == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;

// Failing sequence (stops at second step)
auto failing_sequence = Builder()
    .sequence()
        .action([](Blackboard& bb) -> Status {
            std::cout << "1️⃣ Attempt to open door" << std::endl;
            return Status::Success;
        })
        .action([](Blackboard& bb) -> Status {
            std::cout << "2️⃣ Check if door is locked" << std::endl;
            std::cout << "🔒 Door is locked!" << std::endl;
            return Status::Failure; // This stops the sequence
        })
        .action([](Blackboard& bb) -> Status {
            std::cout << "3️⃣ Walk through door (should not execute)" << std::endl;
            return Status::Success;
        })
    .end()
    .build();

std::cout << "Executing failing door sequence:" << std::endl;
Status door_result = failing_sequence.tick();
std::cout << "Result: " << (door_result == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;
```

**Key Concepts:**
- Sequences execute children from left to right
- If a child returns `Failure`, the sequence immediately returns `Failure`
- If a child returns `Running`, the sequence returns `Running` and resumes from that child next tick
- If all children return `Success`, the sequence returns `Success`
- Perfect for step-by-step processes

---

## Lesson 4: Selector Nodes - Try This, Or That

Selectors try children in order until one succeeds. They're perfect for fallback behaviors.

```cpp
auto problem_solving = Builder()
    .selector()
        .action([](Blackboard& bb) -> Status {
            std::cout << "🔑 Try using the key" << std::endl;
            auto has_key = bb.get<bool>("has_key");
            if (has_key.value_or(false)) {
                std::cout << "✓ Door unlocked with key!" << std::endl;
                return Status::Success;
            } else {
                std::cout << "✗ No key available" << std::endl;
                return Status::Failure;
            }
        })
        .action([](Blackboard& bb) -> Status {
            std::cout << "🔓 Try picking the lock" << std::endl;
            // Simulate 70% success rate
            bool success = (rand() % 10) < 7;
            if (success) {
                std::cout << "✓ Lock picked successfully!" << std::endl;
                return Status::Success;
            } else {
                std::cout << "✗ Lock picking failed" << std::endl;
                return Status::Failure;
            }
        })
        .action([](Blackboard& bb) -> Status {
            std::cout << "🪓 Break down the door" << std::endl;
            std::cout << "✓ Door broken! (Always works but noisy)" << std::endl;
            return Status::Success; // Always succeeds as last resort
        })
    .end()
    .build();

std::cout << "Attempting to open a locked door (with key):" << std::endl;
problem_solving.blackboard().set("has_key", true);
Status door_open1 = problem_solving.tick();
std::cout << "Result: " << (door_open1 == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;

std::cout << "Attempting to open a locked door (without key):" << std::endl;
problem_solving.blackboard().set("has_key", false);
Status door_open2 = problem_solving.tick();
std::cout << "Result: " << (door_open2 == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;
```

**Key Concepts:**
- Selectors try children from left to right
- If a child returns `Success`, the selector immediately returns `Success`
- If a child returns `Running`, the selector returns `Running` and resumes from that child next tick
- If all children return `Failure`, the selector returns `Failure`
- Perfect for fallback strategies and alternatives

---

## Lesson 5: Parallel Nodes - Do Multiple Things

Parallel nodes execute all children simultaneously. They use success and failure policies to determine outcome.

```cpp
auto multitasking = Builder()
    .parallel(Parallel::Policy::RequireOne, Parallel::Policy::RequireAll)
        .action([](Blackboard& bb) -> Status {
            std::cout << "🔍 Scanning for enemies... ";
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            std::cout << "Clear!" << std::endl;
            return Status::Success;
        })
        .action([](Blackboard& bb) -> Status {
            std::cout << "❤️ Monitoring health... ";
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            std::cout << "Healthy!" << std::endl;
            return Status::Success;
        })
        .action([](Blackboard& bb) -> Status {
            std::cout << "📡 Radio check... ";
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            bool radio_works = (rand() % 2) == 0;
            if (radio_works) {
                std::cout << "Connected!" << std::endl;
                return Status::Success;
            } else {
                std::cout << "No signal!" << std::endl;
                return Status::Failure;
            }
        })
    .end()
    .build();

std::cout << "Running parallel tasks:" << std::endl;
Status parallel_result = multitasking.tick();
std::cout << "Result: " << (parallel_result == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;
```

**Key Concepts:**
- Parallel nodes execute all children in the same tick
- Success policy determines when the parallel node succeeds:
  - `RequireOne`: Succeeds when at least one child succeeds
  - `RequireAll`: Succeeds when all children succeed
- Failure policy determines when the parallel node fails:
  - `RequireOne`: Fails when at least one child fails
  - `RequireAll`: Fails when all children fail
- Perfect for simultaneous monitoring and actions

---

## Lesson 6: Decorators - Modify Behavior

Decorators wrap other nodes and modify their behavior. They're like filters or modifiers for node results.

```cpp
// Inverter - flips success/failure
std::cout << "🔄 Inverter Demo:" << std::endl;
auto inverter_demo = Builder()
    .inverter()
        .action([](Blackboard& bb) -> Status {
            std::cout << "Action returns FAILURE" << std::endl;
            return Status::Failure; // Will be inverted to Success
        })
    .end()
    .build();

Status inverted = inverter_demo.tick();
std::cout << "Inverter result: " << (inverted == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;

// Repeater
std::cout << "🔁 Repeater Demo:" << std::endl;
int repeat_counter = 0;
auto repeat_demo = Builder()
    .repeat(3)
        .action([&repeat_counter](Blackboard& bb) -> Status {
            repeat_counter++;
            std::cout << "Execution #" << repeat_counter << std::endl;
            return Status::Failure; // Will be repeated
        })
    .end()
    .build();

repeat_demo.tick();

// Succeeder - always returns success
std::cout << "✅ Succeeder Demo:" << std::endl;
auto succeeder_demo = Builder()
    .succeeder()
        .action([](Blackboard& bb) -> Status {
            std::cout << "Action that fails but succeeder overrides" << std::endl;
            return Status::Failure; // Will become Success
        })
    .end()
    .build();

Status succ_result = succeeder_demo.tick();
std::cout << "Succeeder result: " << (succ_result == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;
```

**Available Decorators:**
- `inverter()`: Flips Success ↔ Failure (Running unchanged)
- `repeater(n)`: Repeats child up to n times (-1 for infinite)
- `succeeder()`: Always returns Success regardless of child result
- `failer()`: Always returns Failure regardless of child result

---

## Lesson 7: Real-World Example - AI Guard

Let's build a complete AI guard behavior that demonstrates how all these concepts work together in practice.

```cpp
auto guard_ai = Builder()
    .selector() // Try different strategies
        // High priority: Handle emergencies
        .sequence()
            .action([](Blackboard& bb) -> Status {
                auto alert_level = bb.get<int>("alert_level");
                if (alert_level.value_or(0) >= 3) {
                    std::cout << "🚨 HIGH ALERT: Intruder detected!" << std::endl;
                    return Status::Success;
                }
                return Status::Failure;
            })
            .action([](Blackboard& bb) -> Status {
                std::cout << "📞 Calling for backup" << std::endl;
                bb.set("backup_called", true);
                return Status::Success;
            })
        .end()
        
        // Medium priority: Investigate disturbances
        .sequence()
            .action([](Blackboard& bb) -> Status {
                auto alert_level = bb.get<int>("alert_level");
                if (alert_level.value_or(0) >= 2) {
                    std::cout << "🔍 Disturbance detected" << std::endl;
                    return Status::Success;
                }
                return Status::Failure;
            })
            .action([](Blackboard& bb) -> Status {
                std::cout << "🚶 Moving to investigate" << std::endl;
                bb.set("investigating", true);
                return Status::Success;
            })
        .end()
        
        // Low priority: Normal patrol
        .sequence()
            .action([](Blackboard& bb) -> Status {
                std::cout << "🚶‍♂️ Normal patrol" << std::endl;
                return Status::Success;
            })
            .parallel(Parallel::Policy::RequireOne, Parallel::Policy::RequireAll)
                .action([](Blackboard& bb) -> Status {
                    std::cout << "   👀 Scanning area" << std::endl;
                    return Status::Success;
                })
                .action([](Blackboard& bb) -> Status {
                    std::cout << "   📝 Logging patrol" << std::endl;
                    return Status::Success;
                })
                .action([](Blackboard& bb) -> Status {
                    std::cout << "   📡 Radio check" << std::endl;
                    return rand() % 2 ? Status::Success : Status::Failure;
                })
            .end()
        .end()
    .end()
    .build();

// Simulate different scenarios
std::vector<int> scenarios = {0, 2, 3}; // Normal, disturbance, high alert
std::vector<std::string> scenario_names = {"Normal", "Disturbance", "High Alert"};

for (size_t i = 0; i < scenarios.size(); ++i) {
    std::cout << "--- Scenario: " << scenario_names[i] << " ---" << std::endl;
    guard_ai.blackboard().set("alert_level", scenarios[i]);
    Status guard_result = guard_ai.tick();
    std::cout << "Guard AI result: " << (guard_result == Status::Success ? "SUCCESS" : "FAILURE") << std::endl;
}
```

This example demonstrates:
- **Priority-based behavior**: Higher priority actions are checked first
- **State-driven logic**: The alert level drives different behaviors
- **Complex coordination**: Multiple systems working together
- **Realistic AI patterns**: Emergency response, investigation, and routine patrol

---

## Lesson 8: Manual Node Creation

For advanced cases, you can create nodes manually and use utility-based selection for sophisticated AI behavior.

```cpp
// Create a utility selector manually
auto utilitySelector = std::make_shared<UtilitySelector>();

// Add children with utility functions
utilitySelector->addChild(
    std::make_shared<Action>([](Blackboard& bb) -> Status {
        std::cout << "🍎 Eating action selected (high hunger utility)" << std::endl;
        return Status::Success;
    }),
    [](Blackboard& bb) -> float {
        return bb.get<double>("hunger").value_or(0.0);
    }
);

utilitySelector->addChild(
    std::make_shared<Action>([](Blackboard& bb) -> Status {
        std::cout << "😴 Sleeping action selected (high tiredness utility)" << std::endl;
        return Status::Success;
    }),
    [](Blackboard& bb) -> float {
        return bb.get<double>("tiredness").value_or(0.0);
    }
);

Tree utility_tree(utilitySelector);

// Test with different utility values
std::cout << "Testing utility-based selection:" << std::endl;

utility_tree.blackboard().set("hunger", 0.8);
utility_tree.blackboard().set("tiredness", 0.3);
std::cout << "High hunger, low tiredness:" << std::endl;
utility_tree.tick(); // Should select eating

utility_tree.blackboard().set("hunger", 0.2);
utility_tree.blackboard().set("tiredness", 0.9);
std::cout << "Low hunger, high tiredness:" << std::endl;
utility_tree.tick(); // Should select sleeping
```

**Key Concepts:**
- Manual node creation gives you full control
- Utility-based selection chooses actions based on scored utility functions
- Custom node types can implement sophisticated AI behaviors
- Mix manual and builder approaches as needed

---

## Tutorial Complete! 🎉

Congratulations! You've learned how to use behavior trees with Bonsai!

### Key concepts covered:
✓ Actions and Status values  
✓ Blackboard for shared data  
✓ Sequence nodes (do this, then that)  
✓ Selector nodes (try this, or that)  
✓ Parallel nodes (do multiple things)  
✓ Decorators (modify behavior)  
✓ Real-world AI examples  
✓ Manual node creation and utility-based selection  

### Next steps:
• Check out the other examples in the `examples/` directory  
• Read the full API documentation in `README.md`  
• Start building your own behavior trees!  

### Running the Interactive Tutorial

You can run the complete interactive version of this tutorial:

```bash
# Build the examples
cmake -DBONSAI_BUILD_EXAMPLES=ON . -B build
cmake --build build

# Run the interactive tutorial
./build/getting_started_tutorial
```

Happy coding! 🌳
