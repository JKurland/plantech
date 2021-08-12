#include <benchmark/benchmark.h>

#include "queues/mpsc.h"

#include <thread>
#include <atomic>
#include <random>

using namespace pt;

static void BM_MpscQueue(benchmark::State& state) {
    for (auto _ : state) {
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

    }
}


BENCHMARK(BM_MpscQueue);

static void BM_MpscQueueNoThreads(benchmark::State& state) {
    for (auto _ : state) {
        std::default_random_engine gen(78);
        std::bernoulli_distribution dist(0.5);
        MpscQueue<size_t> q;
        for (size_t i = 0; i < 100000; i++) {
            if (dist(gen)) {
                q.push(i);
            } else if (!q.empty()) {
                benchmark::DoNotOptimize(q.pop());
            }
        }

        while (!q.empty()) {
            q.pop();
        }
    }
}

BENCHMARK(BM_MpscQueueNoThreads);
