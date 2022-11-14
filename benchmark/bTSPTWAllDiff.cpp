#include <benchmark/benchmark.h>

#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "benchmark.hpp"
#include "constraints/lessEqual.hpp"
#include "core/propagationEngine.hpp"
#include "invariants/element2dConst.hpp"
#include "invariants/linear.hpp"
#include "invariants/plus.hpp"
#include "views/intOffsetView.hpp"
#include "views/lessEqualConst.hpp"

inline bool all_in_range(size_t start, size_t stop,
                         std::function<bool(size_t)> predicate) {
  std::vector<size_t> vec(stop - start);
  std::iota(vec.begin(), vec.end(), start);
  return std::all_of(vec.begin(), vec.end(), predicate);
}

class TSPTWAllDiff : public benchmark::Fixture {
 public:
  std::unique_ptr<PropagationEngine> engine;
  std::vector<VarId> tour;
  std::vector<VarId> timeTo;
  std::vector<VarId> arrivalTime;
  std::vector<std::vector<Int>> dist;
  VarId totalDist;

  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<Int> distribution;
  Int n;
  const int MAX_TIME = 100000;

  std::vector<VarId> violation;
  VarId totalViolation;

  void SetUp(const ::benchmark::State& state) override {
    engine = std::make_unique<PropagationEngine>();
    // First location is the dummy location:
    n = state.range(0);

    if (n < 3) {
      throw std::runtime_error("There must be at least 3 locations.");
    }

    engine->open();

    setEngineModes(*engine, state.range(1));

    for (int i = 1; i <= n; ++i) {
      dist.emplace_back();
      for (int j = 1; j <= n; ++j) {
        dist.back().push_back(i * j);
      }
    }

    tour.emplace_back(engine->makeIntVar(0, 0, n - 1));
    timeTo.emplace_back(NULL_ID);
    arrivalTime.emplace_back(engine->makeIntVar(0, 0, 0));

    for (int i = 1; i < n; ++i) {
      tour.emplace_back(engine->makeIntVar(i, 0, n - 1));
      timeTo.emplace_back(engine->makeIntVar(0, 0, MAX_TIME));
      arrivalTime.emplace_back(engine->makeIntVar(0, 0, MAX_TIME));
    }

    assert(all_in_range(0, n - 1, [&](const size_t a) {
      const Int curA = engine->currentValue(tour.at(a));
      return all_in_range(a + 1, n, [&](const size_t b) {
        const Int curB = engine->currentValue(tour.at(b));
        if (curA == curB) {
          logDebug("a: " << a << "; b: " << b);
          return false;
        }
        return curA != curB;
      });
    }));

    // Ignore index 0
    for (int i = 1; i < n; ++i) {
      // timeTo[i] = dist[tour[i - 1]][tour[i]]
      engine->makeInvariant<Element2dConst>(*engine, timeTo[i], tour[i - 1],
                                            tour[i], dist, 0, 0);
      // arrivalTime[i] = arrivalTime[i - 1] + timeTo[i];
      engine->makeInvariant<Plus>(*engine, arrivalTime[i], arrivalTime[i - 1],
                                  timeTo[i]);
    }

    // remove dummy from distance:
    timeTo.erase(timeTo.begin());
    // totalDist = sum(timeTo)
    totalDist = engine->makeIntVar(0, 0, MAX_TIME);
    engine->makeInvariant<Linear>(*engine, totalDist, timeTo);

    for (int i = 1; i < n; ++i) {
      violation.emplace_back(
          engine->makeIntView<LessEqualConst>(*engine, arrivalTime[i], 100));
    }

    totalViolation = engine->makeIntVar(0, 0, MAX_TIME * n);
    engine->makeInvariant<Linear>(*engine, totalViolation, violation);

    engine->close();
    assert(std::all_of(tour.begin(), tour.end(), [&](const VarId p) {
      return engine->lowerBound(p) == 0;
    }));
    assert(std::all_of(tour.begin(), tour.end(), [&](const VarId p) {
      return engine->upperBound(p) == n - 1;
    }));
    assert(std::all_of(tour.begin(), tour.end(), [&](const VarId p) {
      return 0 <= engine->committedValue(p) &&
             engine->committedValue(p) <= n - 1;
    }));

    gen = std::mt19937(rd());

    distribution = std::uniform_int_distribution<Int>{0, n - 1};
  }

  void TearDown(const ::benchmark::State&) override {
    tour.clear();
    timeTo.clear();
    arrivalTime.clear();
    dist.clear();
    violation.clear();
  }

  Int computeDistance() {
    Int tot = 0;
    // Ignore wrapping from last to first location
    for (Int i = 1; i < n; ++i) {
      tot += dist.at(engine->currentValue(tour.at(i - 1)))
                 .at(engine->currentValue(tour.at(i)));
    }
    return tot;
  }
};

inline size_t rand_in_range(size_t min, size_t max) {
  std::random_device rd;
  std::mt19937 rng(rd());
  return std::uniform_int_distribution<size_t>(min,
                                               max)(rng);  // uniform, unbiased
}

BENCHMARK_DEFINE_F(TSPTWAllDiff, probe_three_opt)(benchmark::State& st) {
  size_t probes = 0;
  assert(all_in_range(0, n, [&](const size_t a) {
    const Int curA = engine->committedValue(tour.at(a));
    return all_in_range(a + 1, n, [&](const size_t b) {
      const Int curB = engine->committedValue(tour.at(b));
      if (curA == curB) {
        logDebug("a: " << a << "; b: " << b);
        return false;
      }
      return curA != curB;
    });
  }));

  for (auto _ : st) {
    const size_t b = rand_in_range(0, n - 2);
    const size_t d = rand_in_range(b + 1, n - 1);
    const size_t e = rand_in_range(d, n - 1);

    assert(b < d);
    assert(d <= e);
    assert(e < static_cast<size_t>(n));

    engine->beginMove();
    size_t cur = b;
    logDebug("probe " << probes);
    for (size_t i = d; i <= e; ++i) {
      logDebug("  tour[" << cur << "] = tour[" << i << ']');
      engine->setValue(tour[cur++], engine->committedValue(tour[i]));
    }
    for (size_t i = b; i < d; ++i) {
      logDebug("  tour[" << cur << "] = tour[" << i << ']');
      engine->setValue(tour[cur++], engine->committedValue(tour[i]));
    }

    assert(all_in_range(0, n, [&](const size_t i) {
      const Int curI = engine->currentValue(tour.at(i));
      return all_in_range(i + 1, n, [&](const size_t j) {
        const Int curJ = engine->currentValue(tour.at(j));
        if (curI == curJ) {
          logDebug("i: " << i << "; j: " << j);
          return false;
        }
        return curI != curJ;
      });
    }));
    engine->endMove();

    engine->beginProbe();
    engine->query(totalDist);
    engine->query(totalViolation);
    engine->endProbe();
    assert(engine->currentValue(totalDist) == computeDistance());
    ++probes;
  }
  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(TSPTWAllDiff, probe_all_relocate)(benchmark::State& st) {
  size_t probes = 0;
  for (auto _ : st) {
    for (int i = 0; i < n; ++i) {
      for (int j = i + 1; j < n; ++j) {
        engine->beginMove();
        engine->setValue(tour[i], engine->committedValue(tour[j]));
        engine->setValue(tour[j], engine->committedValue(tour[i]));
        engine->endMove();

        engine->beginProbe();
        engine->query(totalDist);
        engine->query(totalViolation);
        engine->endProbe();
        ++probes;
      }
    }
  }
  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
}

//*
static void arguments(benchmark::internal::Benchmark* b) {
  int i = 16;
  while (i <= 1024) {
    for (int mode = 0; mode <= 3; ++mode) {
      b->Args({i, mode});
    }
    if (i < 128) {
      i += 16;
      continue;
    } else if (i < 256) {
      i += 64;
      continue;
    }
    i *= 2;
#ifndef NDEBUG
    return;
#endif
  }
}

BENCHMARK_REGISTER_F(TSPTWAllDiff, probe_three_opt)
    ->Apply(arguments)
    ->Unit(benchmark::kMillisecond);

/*
BENCHMARK_REGISTER_F(TSPTWAllDiff, probe_all_relocate)
    ->Apply(arguments)
    ->Unit(benchmark::kMillisecond);
//*/