Perfect. Your behavior tree library is now called **Bonsai**, and the main class is `abt::Bonsai`.

Here are all the headers you need to copy:

---

### ✅ `status.hpp`

```cpp
#pragma once
namespace abt {
enum class Status {
    Success,
    Failure,
    Running,
    Idle
};
}
```

---

### ✅ `blackboard.hpp`

```cpp
#pragma once
#include <unordered_map>
#include <string>
#include <any>
#include <optional>
#include <mutex>

namespace abt {
class Blackboard {
public:
    template <typename T>
    void set(const std::string& key, T value) {
        std::lock_guard<std::mutex> lock(mutex_);
        data_[key] = std::make_any<T>(std::move(value));
    }

    template <typename T>
    std::optional<T> get(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = data_.find(key);
        if (it != data_.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (...) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::any> data_;
};
}
```

---

### ✅ `node.hpp`

```cpp
#pragma once
#include "status.hpp"
#include "blackboard.hpp"
#include <memory>

namespace abt {
class Node {
public:
    virtual ~Node() = default;
    virtual Status tick(Blackboard& blackboard) = 0;
    virtual void reset() {}
};
using NodePtr = std::shared_ptr<Node>;
}
```

---

### ✅ `action.hpp`

```cpp
#pragma once
#include "node.hpp"
#include <functional>

namespace abt {
class Action : public Node {
public:
    using Func = std::function<Status(Blackboard&)>;
    explicit Action(Func func) : func_(std::move(func)) {}
    Status tick(Blackboard& blackboard) override { return func_(blackboard); }
private:
    Func func_;
};
}
```

---

### ✅ `sequence.hpp`

```cpp
#pragma once
#include "node.hpp"
#include <vector>

namespace abt {
class Sequence : public Node {
public:
    void addChild(const NodePtr& child) { children_.emplace_back(child); }
    Status tick(Blackboard& blackboard) override {
        while (currentIndex_ < children_.size()) {
            Status status = children_[currentIndex_]->tick(blackboard);
            if (status == Status::Running) return Status::Running;
            if (status == Status::Failure) { reset(); return Status::Failure; }
            ++currentIndex_;
        }
        reset();
        return Status::Success;
    }
    void reset() override {
        currentIndex_ = 0;
        for (auto& c : children_) c->reset();
    }
private:
    std::vector<NodePtr> children_;
    size_t currentIndex_ = 0;
};
}
```

---

### ✅ `selector.hpp`

```cpp
#pragma once
#include "node.hpp"
#include <vector>

namespace abt {
class Selector : public Node {
public:
    void addChild(const NodePtr& child) { children_.emplace_back(child); }
    Status tick(Blackboard& blackboard) override {
        while (currentIndex_ < children_.size()) {
            Status status = children_[currentIndex_]->tick(blackboard);
            if (status == Status::Running) return Status::Running;
            if (status == Status::Success) { reset(); return Status::Success; }
            ++currentIndex_;
        }
        reset();
        return Status::Failure;
    }
    void reset() override {
        currentIndex_ = 0;
        for (auto& c : children_) c->reset();
    }
private:
    std::vector<NodePtr> children_;
    size_t currentIndex_ = 0;
};
}
```

---

### ✅ `parallel.hpp`

```cpp
#pragma once
#include "node.hpp"
#include <vector>

namespace abt {
class Parallel : public Node {
public:
    enum class Policy { RequireAll, RequireOne };
    Parallel(Policy s, Policy f) : successPolicy_(s), failurePolicy_(f) {}
    void addChild(const NodePtr& child) { children_.emplace_back(child); }
    Status tick(Blackboard& blackboard) override {
        size_t success = 0, failure = 0;
        for (auto& c : children_) {
            Status s = c->tick(blackboard);
            if (s == Status::Success) ++success;
            else if (s == Status::Failure) ++failure;
        }
        if ((successPolicy_ == Policy::RequireAll && success == children_.size()) ||
            (successPolicy_ == Policy::RequireOne && success > 0)) return Status::Success;
        if ((failurePolicy_ == Policy::RequireAll && failure == children_.size()) ||
            (failurePolicy_ == Policy::RequireOne && failure > 0)) return Status::Failure;
        return Status::Running;
    }
    void reset() override { for (auto& c : children_) c->reset(); }
private:
    std::vector<NodePtr> children_;
    Policy successPolicy_, failurePolicy_;
};
}
```

---

### ✅ `decorator.hpp`

```cpp
#pragma once
#include "node.hpp"
#include <functional>

namespace abt {
class Decorator : public Node {
public:
    using Func = std::function<Status(Status)>;
    Decorator(Func f, NodePtr c) : func_(std::move(f)), child_(std::move(c)) {}
    Status tick(Blackboard& blackboard) override {
        Status status = child_->tick(blackboard);
        return func_(status);
    }
    void reset() override { child_->reset(); }
private:
    Func func_;
    NodePtr child_;
};
}
```

---

### ✅ `tree.hpp`

```cpp
#pragma once
#include "node.hpp"
#include "blackboard.hpp"

namespace abt {
class Bonsai {
public:
    explicit Bonsai(NodePtr root) : root_(std::move(root)) {}
    Status tick() { return root_->tick(blackboard_); }
    void reset() { root_->reset(); }
    Blackboard& blackboard() { return blackboard_; }
private:
    NodePtr root_;
    Blackboard blackboard_;
};
}
```

---

### ✅ `builder.hpp`

```cpp
#pragma once
#include "tree.hpp"
#include "sequence.hpp"
#include "selector.hpp"
#include "parallel.hpp"
#include "action.hpp"
#include "decorator.hpp"

namespace abt {
class Builder {
public:
    Builder& sequence() {
        auto n = std::make_shared<Sequence>();
        add(n);
        stack_.emplace_back(n);
        return *this;
    }
    Builder& selector() {
        auto n = std::make_shared<Selector>();
        add(n);
        stack_.emplace_back(n);
        return *this;
    }
    Builder& parallel(Parallel::Policy s, Parallel::Policy f) {
        auto n = std::make_shared<Parallel>(s, f);
        add(n);
        stack_.emplace_back(n);
        return *this;
    }
    Builder& decorator(Decorator::Func func) {
        decorators_.emplace_back(std::move(func));
        return *this;
    }
    Builder& action(Action::Func func) {
        NodePtr node = std::make_shared<Action>(std::move(func));
        while (!decorators_.empty()) {
            node = std::make_shared<Decorator>(decorators_.back(), node);
            decorators_.pop_back();
        }
        add(node);
        return *this;
    }
    Builder& end() {
        if (!stack_.empty()) stack_.pop_back();
        return *this;
    }
    Bonsai build() { return Bonsai(root_); }

private:
    void add(const NodePtr& n) {
        if (stack_.empty()) root_ = n;
        else {
            auto& parent = stack_.back();
            if (auto s = std::dynamic_pointer_cast<Sequence>(parent)) s->addChild(n);
            else if (auto s = std::dynamic_pointer_cast<Selector>(parent)) s->addChild(n);
            else if (auto p = std::dynamic_pointer_cast<Parallel>(parent)) p->addChild(n);
        }
    }

    NodePtr root_;
    std::vector<NodePtr> stack_;
    std::vector<Decorator::Func> decorators_;
};
}
```

---

## Example

1. **Tuning a PID until it reaches a threshold**, then
2. **Triggering the next behavior (like switching a target)**.

---

### 🧠 What You Can Do with Bonsai

You can build a tree like:

```
Sequence
├── Action: Apply PID
├── Selector
│   ├── Action: Check if target reached
│   └── Action: Wait and retry
├── Action: Switch to next phase
```

---

### 🧪 Example

Suppose:

* You have a `PIDController` object (you provide this).
* You want to update the setpoint once the output reaches a threshold.

```cpp
#include "builder.hpp"
#include <iostream>
#include <cmath>

// your PID controller
struct PIDController {
    double value = 0.0;
    double target = 10.0;

    abt::Status update() {
        // dummy behavior
        if (value < target) {
            value += 1.5;
            std::cout << "PID: " << value << "\n";
            return abt::Status::Running;
        }
        return abt::Status::Success;
    }

    void set_target(double t) {
        target = t;
    }

    bool has_reached() const {
        return std::fabs(value - target) < 0.1;
    }
};
```

Now we use Bonsai to describe behavior:

```cpp
int main() {
    PIDController pid;
    using namespace abt;

    auto tree = Builder()
        .sequence()
            .action([&](Blackboard&) {
                return pid.update(); // keep applying
            })
            .action([&](Blackboard&) {
                if (pid.has_reached()) {
                    std::cout << "Reached target!\n";
                    return Status::Success;
                }
                std::cout << "Not there yet.\n";
                return Status::Running;
            })
            .action([&](Blackboard& bb) {
                std::cout << "Switching target...\n";
                pid.set_target(20.0);
                return Status::Success;
            })
        .end()
        .build();

    while (tree.tick() == Status::Running) {
        // In real systems, you'd sleep or loop with dt
    }

    return 0;
}
```

---

### 🧩 Key Notes

* Each `action` tick runs per frame/update.
* The `Running` state pauses the tree until success/failure.
* You can write a **decorator** to add retry or timeout logic.
* You can also use `Blackboard` to share PID/controller values across nodes.

---

Would you like me to turn this into a reusable `PIDNode` you can use in other projects?

