#include <benchmark/benchmark.h>

#include <iostream>
#include <memory>
#include <random>
#include <utility>
#include <vector>

#include "propagation/propagation/propagationQueue.hpp"

namespace atlantis::benchmark {

class PropQueue : public ::benchmark::Fixture {
 public:
  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<> distribution;
  size_t queueSize{0};

  void SetUp(const ::benchmark::State& st) override {
    queueSize = size_t(st.range(0));

    gen = std::mt19937(rd());

    distribution = std::uniform_int_distribution<>{1, int(queueSize)};
  }
};

BENCHMARK_DEFINE_F(PropQueue, initVar)(::benchmark::State& st) {
  size_t inits = 0;
  for (const auto& _ : st) {
    propagation::PropagationQueue queue;
    for (size_t i = 1; i <= queueSize; ++i) {
      queue.initVar(i, i);
      ++inits;
    }
  }
  st.counters["inits_per_second"] =
      ::benchmark::Counter(static_cast<double>(inits), ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(PropQueue, push_min)(::benchmark::State& st) {
  size_t pushes = 0;
  for (const auto& _ : st) {
    st.PauseTiming();
    propagation::PropagationQueue queue;
    for (size_t i = 1; i <= queueSize; ++i) {
      queue.initVar(i, i);
    }
    st.ResumeTiming();

    for (size_t i = 1; i <= queueSize; ++i) {
      queue.push(i);
      ++pushes;
    }
  }
  st.counters["pushes_per_second"] =
      ::benchmark::Counter(static_cast<double>(pushes), ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(PropQueue, push_max)(::benchmark::State& st) {
  size_t pushes = 0;
  for (const auto& _ : st) {
    st.PauseTiming();
    propagation::PropagationQueue queue;
    for (size_t i = 1; i <= queueSize; ++i) {
      queue.initVar(i, i);
    }
    st.ResumeTiming();

    for (size_t i = queueSize; i > 0; --i) {
      queue.push(i);
      ++pushes;
    }
  }
  st.counters["pushes_per_second"] =
      ::benchmark::Counter(static_cast<double>(pushes), ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(PropQueue, push_random)(::benchmark::State& st) {
  size_t pushes = 0;
  for (const auto& _ : st) {
    st.PauseTiming();
    propagation::PropagationQueue queue;
    for (size_t i = 1; i <= queueSize; ++i) {
      queue.initVar(i, distribution(gen));
    }
    st.ResumeTiming();

    for (size_t i = 1; i <= queueSize; ++i) {
      queue.push(i);
      ++pushes;
    }
  }
  st.counters["pushes_per_second"] =
      ::benchmark::Counter(static_cast<double>(pushes), ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(PropQueue, pop_min)(::benchmark::State& st) {
  size_t pops = 0;
  for (const auto& _ : st) {
    st.PauseTiming();
    propagation::PropagationQueue queue;
    for (size_t i = 1; i <= queueSize; ++i) {
      queue.initVar(i, i);
    }
    for (size_t i = 1; i <= queueSize; ++i) {
      queue.push(i);
    }
    st.ResumeTiming();

    while (!queue.empty()) {
      queue.pop();
      ++pops;
    }
  }
  st.counters["pops_per_second"] =
      ::benchmark::Counter(static_cast<double>(pops), ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(PropQueue, pop_max)(::benchmark::State& st) {
  size_t pops = 0;
  for (const auto& _ : st) {
    st.PauseTiming();
    propagation::PropagationQueue queue;
    for (size_t i = 1; i <= queueSize; ++i) {
      queue.initVar(i, queueSize - i + 1);
    }
    for (size_t i = 1; i <= queueSize; ++i) {
      queue.push(i);
    }
    st.ResumeTiming();

    while (!queue.empty()) {
      queue.pop();
      ++pops;
    }
  }
  st.counters["pops_per_second"] =
      ::benchmark::Counter(static_cast<double>(pops), ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(PropQueue, pop_random)(::benchmark::State& st) {
  size_t pops = 0;
  for (const auto& _ : st) {
    st.PauseTiming();
    propagation::PropagationQueue queue;
    for (size_t i = 1; i <= queueSize; ++i) {
      queue.initVar(i, distribution(gen));
    }

    for (size_t i = 1; i <= queueSize; ++i) {
      queue.push(i);
    }

    st.ResumeTiming();
    while (!queue.empty()) {
      queue.pop();
      ++pops;
    }
  }
  st.counters["pops_per_second"] =
      ::benchmark::Counter(static_cast<double>(pops), ::benchmark::Counter::kIsRate);
}

// This benchmark is not a model, but mainly to test the performance
// of the propagation::PropagationQueue data structure (it typically does not need
// to be benchmarked)

/*
static void arguments(::benchmark::internal::Benchmark* b) {
  for (int i = 5000; i <= 20000; i *= 10) {
    b->Arg(i);
#ifndef NDEBUG
    return;
#endif
  }
}

BENCHMARK_REGISTER_F(PropQueue, initVar)->Apply(arguments);
BENCHMARK_REGISTER_F(PropQueue, push_min)->Apply(arguments);
BENCHMARK_REGISTER_F(PropQueue, push_max)->Apply(arguments);
BENCHMARK_REGISTER_F(PropQueue, push_random)->Apply(arguments);
BENCHMARK_REGISTER_F(PropQueue, pop_min)->Apply(arguments);
BENCHMARK_REGISTER_F(PropQueue, pop_max)->Apply(arguments);
BENCHMARK_REGISTER_F(PropQueue, pop_random)->Apply(arguments);
//*/
}  // namespace atlantis::benchmark