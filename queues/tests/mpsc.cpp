#include <gtest/gtest.h>
#include "queues/mpsc.h"

#include <atomic>
#include <random>
#include <thread>

using namespace pt;

TEST(MpscQueue, empty_on_construction) {
    MpscQueue<int> q;
    ASSERT_TRUE(q.empty());
}

TEST(MpscQueue, not_empty_after_push) {
    MpscQueue<int> q;
    q.push(1);
    ASSERT_FALSE(q.empty());
}

TEST(MpscQueue, push_twice_not_empty) {
    MpscQueue<int> q;
    q.push(1);
    q.push(3);
    ASSERT_FALSE(q.empty());
}

TEST(MpscQueue, push_and_pop_then_empty) {
    MpscQueue<int> q;
    q.push(1);
    q.pop();
    ASSERT_TRUE(q.empty());
}

TEST(MpscQueue, pop_gets_what_was_pushed) {
    MpscQueue<int> q;
    q.push(1);
    ASSERT_EQ(q.pop(), 1);
}

TEST(MpscQueue, pop_gets_what_was_pushed_in_order) {
    MpscQueue<int> q;
    q.push(1);
    q.push(2);
    ASSERT_EQ(q.pop(), 1);
    ASSERT_EQ(q.pop(), 2);
}

struct D {
    D(std::atomic<size_t>& c, std::atomic<size_t>& d): c(&c), d(&d) {c++;}
    ~D() {(*d)++;}

    D(const D& o): c(o.c), d(o.d) {(*c)++;}

    std::atomic<size_t>* c;
    std::atomic<size_t>* d;
};

class MpscQueueF: public ::testing::Test {
protected:
    D d() {return D(c_count, d_count);}
    std::atomic<size_t> c_count = 0;
    std::atomic<size_t> d_count = 0;
};

TEST_F(MpscQueueF, destructor_gets_called_on_pop) {
    MpscQueue<D> q;
    q.push(D(c_count, d_count));
    q.pop();
    ASSERT_EQ(c_count, d_count);
}

TEST_F(MpscQueueF, destructor_gets_called_with_many_pushes_and_pops) {
    MpscQueue<D> q;
    for (size_t i = 0; i < 10000; i++) {
        q.push(D(c_count, d_count));
    }

    for (size_t i = 0; i < 10000; i++) {
        q.pop();
    }
    ASSERT_EQ(c_count, d_count);
}

TEST_F(MpscQueueF, destructor_gets_called_with_many_pushes_and_pops_interleaved) {
    std::default_random_engine gen(78);
    std::bernoulli_distribution dist(0.5);
    MpscQueue<D> q;
    for (size_t i = 0; i < 100000; i++) {
        if (dist(gen)) {
            q.push(D(c_count, d_count));
        } else if (!q.empty()) {
            q.pop();
        }
    }

    while (!q.empty()) {
        q.pop();
    }

    ASSERT_EQ(c_count, d_count);
}

TEST_F(MpscQueueF, destructor_gets_called_on_pop_threaded) {
    MpscQueue<D> q;

    auto push = [&]{
        for (size_t i = 0; i < 100000; i++) {
            q.push(D(c_count, d_count));
        }
    };

    std::atomic<bool> keep_popping = true;
    auto pop = [&]{
        while (keep_popping || !q.empty()) {
            q.pop();
        }
    };

    std::thread t_pop{pop};
    std::thread t_push1{push};
    std::thread t_push2{push};
    std::thread t_push3{push};

    t_push1.join();
    t_push2.join();
    t_push3.join();

    keep_popping = false;
    q.push(D(c_count, d_count));
    t_pop.join();

    while (!q.empty()) {
        q.pop();
    }

    ASSERT_EQ(c_count, d_count);
}


TEST(MpscQueue, everything_enqueued_is_dequeued_threaded) {
    constexpr size_t iters = 100000;
    MpscQueue<size_t> q;
    q.reserve(iters * 3);

    auto push = [&](size_t id){
        for (size_t i = 0; i < iters; i++) {
            q.push(i+id);
        }
    };

    std::atomic<size_t> thread_counts[] = {0, 0, 0};
    std::atomic<bool> keep_popping = true;

    auto pop = [&]{
        while (keep_popping || !q.empty()) {
            size_t x = q.pop();
            size_t thread_num = x / iters;
            if (thread_counts[thread_num] == (x%iters)-1) {
                thread_counts[thread_num] = x%iters;
            }
        }
    };

    std::thread t_pop{pop};
    std::thread t_push1{push, 0};
    std::thread t_push2{push, iters};
    std::thread t_push3{push, 2*iters};

    t_push1.join();
    t_push2.join();
    t_push3.join();

    keep_popping = false;
    q.push(0);
    t_pop.join();

    while (!q.empty()) {
        q.pop();
    }

    ASSERT_EQ(thread_counts[0], iters-1);
    ASSERT_EQ(thread_counts[1], iters-1);
    ASSERT_EQ(thread_counts[2], iters-1);
}

TEST(MpscQueue, copy_ctor) {
    MpscQueue<int> q;
    q.push(1);

    MpscQueue<int> q2(q);
    MpscQueue<int> q3;
    q3 = q;

    ASSERT_EQ(q.pop(), 1);
    ASSERT_EQ(q2.pop(), 1);
    ASSERT_EQ(q3.pop(), 1);
}


TEST(MpscQueue, move_ctor) {
    MpscQueue<int> q;
    q.push(1);

    MpscQueue<int> q2(std::move(q));
    MpscQueue<int> q3;
    q3 = std::move(q2);

    ASSERT_EQ(q3.pop(), 1);
}

TEST(MpscQueue, push_onto_moved_into_queue) {
    MpscQueue<int> q;
    q.push(1);

    MpscQueue<int> q2 = std::move(q);

    for (size_t i = 0; i < 1000; i++) {
        q2.push(i);
    }

    ASSERT_EQ(q2.pop(), 1);
    for (size_t i = 0; i < 1000; i++) {
        ASSERT_EQ(q2.pop(), i);
    }
}

TEST_F(MpscQueueF, destructor_gets_called_move) {
    MpscQueue<D> q;

    for (size_t i = 0; i < 1000; i++) {
        q.push(d());
    }
    
    {
        MpscQueue<D> q2(std::move(q));
    }

    ASSERT_EQ(c_count, d_count);
}

TEST_F(MpscQueueF, destructor_gets_called_copy) {
    {
        MpscQueue<D> q;

        for (size_t i = 0; i < 1000; i++) {
            q.push(d());
        }
    
        MpscQueue<D> q2(q);
    }

    ASSERT_EQ(c_count, d_count);
}

TEST_F(MpscQueueF, destructor_gets_called_move_assign) {
    MpscQueue<D> q;

    for (size_t i = 0; i < 1000; i++) {
        q.push(d());
    }
    
    {
        MpscQueue<D> q2;
        q2 = std::move(q);
    }

    ASSERT_EQ(c_count, d_count);
}

TEST_F(MpscQueueF, destructor_gets_called_copy_assign) {
    {
        MpscQueue<D> q;

        for (size_t i = 0; i < 1000; i++) {
            q.push(d());
        }
    
        MpscQueue<D> q2;
        q2 = q;
    }

    ASSERT_EQ(c_count, d_count);
}


