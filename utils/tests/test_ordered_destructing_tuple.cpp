#include <gtest/gtest.h>
#include <type_traits>
#include <cstdint>

#include "utils/ordered_destructing_tuple.h"

using namespace pt;

struct MoveOnly {
    MoveOnly(int i): i(i) {}
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&&) = default;

    int i;
};

TEST(TestOrderedDestructingTuple, test_empty_tuple) {
    OrderedDestructingTuple t;
    (void)t;
}

TEST(TestOrderedDestructingTuple, test_single_entry) {
    OrderedDestructingTuple<int> t(2);
    ASSERT_EQ(t.get<0>(), 2);
}

TEST(TestOrderedDestructingTuple, test_2_entries) {
    OrderedDestructingTuple<int, bool> t(2, true);
    ASSERT_EQ(t.get<0>(), 2);
    ASSERT_EQ(t.get<1>(), true);
}

TEST(TestOrderedDestructingTuple, test_2_entries_alignment) {
    OrderedDestructingTuple<bool, int> t(true, 3);
    std::uintptr_t p = reinterpret_cast<std::uintptr_t>(&t.get<1>());
    ASSERT_EQ(p % alignof(int), 0);
}

TEST(TestOrderedDestructingTuple, test_several_entries_alignment) {
    struct Empty {};
    struct S {
        char c;
        double d;
    };

    OrderedDestructingTuple<bool, int, S, double, Empty, int*> t(true, 3, S{'g', 4.5}, 2.5, Empty{}, nullptr);

    std::uintptr_t p;
    
    p = reinterpret_cast<std::uintptr_t>(&t.get<0>());
    ASSERT_EQ(p % alignof(bool), 0);

    p = reinterpret_cast<std::uintptr_t>(&t.get<1>());
    ASSERT_EQ(p % alignof(int), 0);

    p = reinterpret_cast<std::uintptr_t>(&t.get<2>());
    ASSERT_EQ(p % alignof(S), 0);

    p = reinterpret_cast<std::uintptr_t>(&t.get<3>());
    ASSERT_EQ(p % alignof(double), 0);

    p = reinterpret_cast<std::uintptr_t>(&t.get<4>());
    ASSERT_EQ(p % alignof(Empty), 0);

    p = reinterpret_cast<std::uintptr_t>(&t.get<5>());
    ASSERT_EQ(p % alignof(int*), 0);
}

TEST(TestOrderedDestructingTuple, test_destruction_order) {
    struct D {
        int* counter;
        int expected;

        D(int* counter, int expected): counter(counter), expected(expected) {}
        D(const D&) = delete;
        D(D&& o): counter(o.counter), expected(o.expected) {o.counter = nullptr;}

        ~D() {
            if (counter && *counter == expected) *counter+=1;
        }
    };

    int counter = 0;
    {
        OrderedDestructingTuple<D, D> t{D{&counter, 1}, D{&counter, 0}};
    }

    ASSERT_EQ(counter, 2);
}


TEST(TestOrderedDestructingTuple, test_copy_constructor) {
    OrderedDestructingTuple<int, double> t(1, 2.0);
    OrderedDestructingTuple<int, double> t2(t);

    ASSERT_EQ(t2.get<0>(), 1);
    ASSERT_EQ(t2.get<1>(), 2.0);
}


TEST(TestOrderedDestructingTuple, test_move_constructor) {
    OrderedDestructingTuple<MoveOnly> t(MoveOnly{3});
    OrderedDestructingTuple<MoveOnly> t2(std::move(t));

    ASSERT_EQ(t2.get<0>().i, 3);
}

TEST(TestOrderedDestructingTuple, test_tuple_concat) {
    OrderedDestructingTuple<int> t1(2);
    OrderedDestructingTuple<double> t2(4.5);
    OrderedDestructingTuple<int, double> t_concat = t1.concat(t2);

    ASSERT_EQ(t_concat.get<0>(), 2);
    ASSERT_EQ(t_concat.get<1>(), 4.5);
}


TEST(TestOrderedDestructingTuple, test_tuple_concat_move) {
    OrderedDestructingTuple<MoveOnly> t1(MoveOnly(-2));
    OrderedDestructingTuple<MoveOnly> t2(MoveOnly(4));
    OrderedDestructingTuple<MoveOnly, MoveOnly> t_concat = std::move(t1).concat(std::move(t2));

    ASSERT_EQ(t_concat.get<0>().i, -2);
    ASSERT_EQ(t_concat.get<1>().i, 4);
}

