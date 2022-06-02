#include "test_utils/test.h"
#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>

using namespace pt::testing;

template<WorldEntryName Name>
struct GivenAnInt: TestStep<Provides<Name, int>> {
    auto step(const auto world) const {
        return this->worldUpdate(WorldEntry<Name, int>{2});
    }
};

template<WorldEntryName Name>
struct WhenIntIsDoubled: TestStep<Requires<Name, int>> {
    void step(auto world) const {
        world.template getEntry<Name, int>() *= 2;
    }
};

template<WorldEntryName Name>
struct ThenIntIsEven: TestStep<Requires<Name, int>> {
    void step(const auto world) const {
        auto i = world.template getEntry<Name, int>();
        ASSERT_EQ(i % 2, 0);
    }
};

template<WorldEntryName Name>
struct ThenIntIsOdd: TestStep<Requires<Name, int>> {
    void step(const auto world) const {
        auto i = world.template getEntry<Name, int>();
        ASSERT_EQ(i % 2, 1);
    }
};

TEST(TestTestFramework, test_it_compiles) {
    ::pt::testing::Test()
        (GivenAnInt<"A">{})
        (WhenIntIsDoubled<"A">{})
        (ThenIntIsEven<"A">{})
    .run();
}


TEST(TestTestFramework, should_assert_on_test_failure) {
    EXPECT_FATAL_FAILURE(
        ::pt::testing::Test()
            (GivenAnInt<"A">{})
            (WhenIntIsDoubled<"A">{})
            (ThenIntIsOdd<"A">{})
        .run()
    , "");
}


TEST(TestTestFramework, should_fail_when_provides_arent_present) {
    EXPECT_FATAL_FAILURE(
    ::pt::testing::Test()
        (WhenIntIsDoubled<"Hello">{})
        .run()
    , "");
}

TEST(TestTestFramework, should_fail_when_provides_are_in_the_wrong_order) {
    EXPECT_FATAL_FAILURE(
    ::pt::testing::Test()
        (WhenIntIsDoubled<"Hello">{})
        (GivenAnInt<"Hello">{})
        .run()
    , "");
}


