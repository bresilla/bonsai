#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <bonsai/bonsai.hpp>
#include <doctest/doctest.h>

using namespace bonsai;

TEST_CASE("Builder basic functionality") {

    SUBCASE("Simple action tree") {
        auto tree = Builder().action([](Blackboard &) { return Status::Success; }).build();

        CHECK(tree.tick() == Status::Success);
    }

    SUBCASE("Simple sequence") {
        auto tree = Builder()
                        .sequence()
                        .action([](Blackboard &) { return Status::Success; })
                        .action([](Blackboard &) { return Status::Success; })
                        .end()
                        .build();

        CHECK(tree.tick() == Status::Success);
    }

    SUBCASE("Simple selector") {
        auto tree = Builder()
                        .selector()
                        .action([](Blackboard &) { return Status::Failure; })
                        .action([](Blackboard &) { return Status::Success; })
                        .end()
                        .build();

        CHECK(tree.tick() == Status::Success);
    }
}

TEST_CASE("Builder nested structures") {
    Blackboard bb;

    SUBCASE("Nested sequence in selector") {
        auto tree = Builder()
                        .selector()
                        .sequence()
                        .action([](Blackboard &) { return Status::Failure; })
                        .action([](Blackboard &) { return Status::Success; })
                        .end()
                        .action([](Blackboard &) { return Status::Success; })
                        .end()
                        .build();

        // First child (sequence) should fail because first action fails
        // Second child (action) should succeed
        CHECK(tree.tick() == Status::Success);
    }

    SUBCASE("Nested selector in sequence") {
        auto tree = Builder()
                        .sequence()
                        .selector()
                        .action([](Blackboard &) { return Status::Failure; })
                        .action([](Blackboard &) { return Status::Success; })
                        .end()
                        .action([](Blackboard &) { return Status::Success; })
                        .end()
                        .build();

        // First child (selector) should succeed
        // Second child (action) should succeed
        // Overall sequence should succeed
        CHECK(tree.tick() == Status::Success);
    }
}

TEST_CASE("Builder with decorators") {
    Blackboard bb;

    SUBCASE("Inverter decorator") {
        auto tree = Builder().inverter().action([](Blackboard &) { return Status::Failure; }).build();

        // Failure should be inverted to Success
        CHECK(tree.tick() == Status::Success);
    }

    SUBCASE("Succeeder decorator") {
        auto tree = Builder().succeeder().action([](Blackboard &) { return Status::Failure; }).build();

        // Failure should be converted to Success
        CHECK(tree.tick() == Status::Success);
    }

    SUBCASE("Failer decorator") {
        auto tree = Builder().failer().action([](Blackboard &) { return Status::Success; }).build();

        // Success should be converted to Failure
        CHECK(tree.tick() == Status::Failure);
    }
}

TEST_CASE("Builder with blackboard interaction") {

    SUBCASE("Actions modify blackboard") {
        auto tree = Builder()
                        .sequence()
                        .action([](Blackboard &bb) {
                            bb.set("step", 1);
                            return Status::Success;
                        })
                        .action([](Blackboard &bb) {
                            auto step = bb.get<int>("step").value_or(0);
                            bb.set("step", step + 1);
                            return Status::Success;
                        })
                        .action([](Blackboard &bb) {
                            auto step = bb.get<int>("step").value_or(0);
                            bb.set("final_step", step);
                            return Status::Success;
                        })
                        .end()
                        .build();

        CHECK(tree.tick() == Status::Success);

        auto final_step = tree.blackboard().get<int>("final_step");
        REQUIRE(final_step.has_value());
        CHECK(final_step.value() == 2);
    }
}

TEST_CASE("Builder repeat decorator") {
    int execution_count = 0;

    SUBCASE("Repeat successful action limited times") {
        auto tree = Builder()
                        .repeat(3)
                        .action([&execution_count](Blackboard &) -> Status {
                            execution_count++;
                            return Status::Success;
                        })
                        .build();

        CHECK(tree.tick() == Status::Success);
        CHECK(execution_count == 3); // Should repeat 3 times then succeed
    }

    SUBCASE("Repeat successful action indefinitely until external condition") {
        execution_count = 0;
        auto tree = Builder()
                        .repeat()
                        .action([&execution_count](Blackboard &) -> Status {
                            execution_count++;
                            // After 5 executions, simulate external failure condition
                            return (execution_count < 5) ? Status::Success : Status::Failure;
                        })
                        .build();

        // The repeat should run until the action fails
        CHECK(tree.tick() == Status::Failure);
        CHECK(execution_count == 5);
    }

    SUBCASE("Repeat stops on failure") {
        execution_count = 0;
        auto tree = Builder()
                        .repeat(5)
                        .action([&execution_count](Blackboard &) -> Status {
                            execution_count++;
                            return (execution_count < 3) ? Status::Success : Status::Failure;
                        })
                        .build();

        CHECK(tree.tick() == Status::Failure);
        CHECK(execution_count == 3); // Should stop repeating when action fails
    }
}

TEST_CASE("Builder retry decorator") {
    int execution_count = 0;

    SUBCASE("Retry on failure - limited times") {
        auto tree = Builder()
                        .retry(3)
                        .action([&execution_count](Blackboard &) -> Status {
                            execution_count++;
                            return Status::Failure; // Always fail to trigger retry
                        })
                        .build();

        CHECK(tree.tick() == Status::Failure);
        CHECK(execution_count == 3); // Should retry 3 times then give up
    }

    SUBCASE("Retry until success") {
        execution_count = 0;
        auto tree = Builder()
                        .retry()
                        .action([&execution_count](Blackboard &) -> Status {
                            execution_count++;
                            return (execution_count < 3) ? Status::Failure : Status::Success;
                        })
                        .build();

        CHECK(tree.tick() == Status::Success);
        CHECK(execution_count == 3); // Should retry until success
    }
}

TEST_CASE("Builder error handling") {
    SUBCASE("Build without root") {
        Builder builder;
        CHECK_THROWS_AS(builder.build(), std::runtime_error);
    }
}
