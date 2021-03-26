/*
#include <benchmark/benchmark.h>

#include <iostream>
#include <random>
#include <utility>
#include <vector>
#include <memory>
#include <queue>
#include <ext/pb_ds/priority_queue.hpp>

class GccPriorityQueue : public benchmark::Fixture {
 public:
  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<> distribution;
  size_t queueSize;
  std::vector<size_t> order;

  struct PriorityCmp {
    std::vector<size_t>& order;
    explicit PriorityCmp(std::vector<size_t>& o) : order(o) {}
    bool operator()(size_t left, size_t right) {
      return order[left] > order[right];
    }
  };

  void SetUp(const ::benchmark::State& st) {
    queueSize = size_t(st.range(0));

    gen = std::mt19937(rd());

    distribution = std::uniform_int_distribution<>{0, int(queueSize) * 2};

    order.resize(queueSize);
  }

  void TearDown(const ::benchmark::State&) {
    order.clear();
  }
};


BENCHMARK_DEFINE_F(GccPriorityQueue, push_min)(benchmark::State& st) {
  size_t pushes = 0;
  for (auto _ : st) {
      std::unique_ptr<__gnu_pbds::priority_queue<size_t, PriorityCmp,
__gnu_pbds::thin_heap_tag>> queue =
std::make_unique<__gnu_pbds::priority_queue<size_t, PriorityCmp,
__gnu_pbds::thin_heap_tag>>(PriorityCmp(order))
      ;
    for (size_t i = 0; i < queueSize; ++i) {
      queue->push(i);
      order[i] = i;
      ++pushes;
    }
  }
  st.counters["pushes_per_second"] = benchmark::Counter(
      pushes, benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(GccPriorityQueue, push_max)(benchmark::State& st) {
  size_t pushes = 0;
  for (auto _ : st) {
    std::unique_ptr<__gnu_pbds::priority_queue<size_t, PriorityCmp,
__gnu_pbds::thin_heap_tag>> queue =
std::make_unique<__gnu_pbds::priority_queue<size_t, PriorityCmp,
__gnu_pbds::thin_heap_tag>>(PriorityCmp(order))
      ;
    for (size_t i = 0; i < queueSize; ++i) {
      queue->push(i);
      order[i] = (queueSize - i - 1);
      ++pushes;
    }
  }
  st.counters["pushes_per_second"] = benchmark::Counter(
      pushes, benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(GccPriorityQueue, push_random)(benchmark::State& st) {
  size_t pushes = 0;
  for (auto _ : st) {
    std::unique_ptr<__gnu_pbds::priority_queue<size_t, PriorityCmp,
__gnu_pbds::thin_heap_tag>> queue =
std::make_unique<__gnu_pbds::priority_queue<size_t, PriorityCmp,
__gnu_pbds::thin_heap_tag>>(PriorityCmp(order))
      ;
    for (size_t i = 0; i < queueSize; ++i) {
      queue->push(i);
      order[i] = distribution(gen);
      ++pushes;
    }
  }
  st.counters["pushes_per_second"] = benchmark::Counter(
      pushes, benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(GccPriorityQueue, pop_min)(benchmark::State& st) {
  size_t pops = 0;
  for (auto _ : st) {
    std::unique_ptr<__gnu_pbds::priority_queue<size_t, PriorityCmp,
__gnu_pbds::thin_heap_tag>> queue =
std::make_unique<__gnu_pbds::priority_queue<size_t, PriorityCmp,
__gnu_pbds::thin_heap_tag>>(PriorityCmp(order))
      ;
    st.PauseTiming();
    for (size_t i = 0; i < queueSize; ++i) {
      queue->push(i);
      order[i] = i;
    }
    st.ResumeTiming();
    while (queue->size() > 0) {
      queue->pop();
      ++pops;
    }
  }
  st.counters["pops_per_second"] = benchmark::Counter(
      pops, benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(GccPriorityQueue, pop_max)(benchmark::State& st) {
  size_t pops = 0;
  for (auto _ : st) {
    std::unique_ptr<__gnu_pbds::priority_queue<size_t, PriorityCmp,
__gnu_pbds::thin_heap_tag>> queue =
std::make_unique<__gnu_pbds::priority_queue<size_t, PriorityCmp,
__gnu_pbds::thin_heap_tag>>(PriorityCmp(order))
      ;
    st.PauseTiming();
    for (size_t i = 0; i < queueSize; ++i) {
      queue->push(i);
      order[i] = queueSize - i - 1;
    }
    st.ResumeTiming();
    while (queue->size() > 0) {
      queue->pop();
      ++pops;
    }
  }
  st.counters["pops_per_second"] = benchmark::Counter(
      pops, benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(GccPriorityQueue, pop_random)(benchmark::State& st) {
  size_t pops = 0;
  for (auto _ : st) {
    std::unique_ptr<__gnu_pbds::priority_queue<size_t, PriorityCmp,
__gnu_pbds::thin_heap_tag>> queue =
std::make_unique<__gnu_pbds::priority_queue<size_t, PriorityCmp,
__gnu_pbds::thin_heap_tag>>(PriorityCmp(order))
      ;
    st.PauseTiming();
    for (size_t i = 0; i < queueSize; ++i) {
      queue->push(i);
      order[i] = distribution(gen);

    }
    st.ResumeTiming();
    while (queue->size() > 0) {
      queue->pop();
      ++pops;
    }
  }
  st.counters["pops_per_second"] = benchmark::Counter(
      pops, benchmark::Counter::kIsRate);
}

static void CustomArguments(benchmark::internal::Benchmark* b) {
  for (size_t i = 10000; i <= 10000000; i*=10) {
      b->Arg(i);
  }
}

BENCHMARK_REGISTER_F(GccPriorityQueue, push_min)->Apply(CustomArguments);
BENCHMARK_REGISTER_F(GccPriorityQueue, push_max)->Apply(CustomArguments);
BENCHMARK_REGISTER_F(GccPriorityQueue, push_random)->Apply(CustomArguments);
BENCHMARK_REGISTER_F(GccPriorityQueue, pop_min)->Apply(CustomArguments);
BENCHMARK_REGISTER_F(GccPriorityQueue, pop_max)->Apply(CustomArguments);
BENCHMARK_REGISTER_F(GccPriorityQueue, pop_random)->Apply(CustomArguments);
//

*/