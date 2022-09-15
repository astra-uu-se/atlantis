#include <benchmark/benchmark.h>

#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "benchmark.hpp"
#include "constraints/lessEqual.hpp"
#include "core/propagationEngine.hpp"
#include "invariants/elementVar.hpp"
#include "invariants/linear.hpp"
#include "views/elementConst.hpp"
#include "views/intOffsetView.hpp"

class TSPTW : public benchmark::Fixture {
 public:
  std::unique_ptr<PropagationEngine> engine;
  std::vector<VarId> pred;
  std::vector<VarId> timeToPrev;
  std::vector<VarId> arrivalPrev;
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
    n = state.range(0);

    if (n < 1) {
      throw std::runtime_error("n must strictly positive.");
    }

    engine->open();

    setEngineModes(*engine, state.range(1));

    for (int i = 0; i < n; ++i) {
      dist.emplace_back();
      for (int j = 0; j < n; ++j) {
        dist[i].push_back(i * j);
      }
    }

    for (int i = 1; i <= n; ++i) {
      const Int initVal = 1 + (i % n);
      pred.emplace_back(engine->makeIntVar(initVal, 1, n));
      timeToPrev.emplace_back(engine->makeIntVar(0, 0, MAX_TIME));
      arrivalTime.emplace_back(engine->makeIntVar(0, 0, MAX_TIME));
      arrivalPrev.emplace_back(engine->makeIntVar(0, 0, MAX_TIME));
      violation.emplace_back(engine->makeIntVar(0, 0, MAX_TIME));
    }

    // Ignore index 0
    for (int i = 1; i < n; ++i) {
      // timeToPrev[i] = dist[i][pred[i]]
      timeToPrev[i] = engine->makeIntView<ElementConst>(pred[i], dist[i]);
      // arrivalPrev[i] = arrivalTime[pred[i]]
    }

    // Ignore index 0
    for (int i = 1; i < n; ++i) {
      // arrivalPrev[i] = arrivalTime[pred[i]]
      engine->makeInvariant<ElementVar>(arrivalPrev[i], pred[i], arrivalTime);
      // arrivalTime[i] = arrivalPrev[i] + timeToPrev[i]
      engine->makeInvariant<Linear>(
          arrivalTime[i], std::vector<VarId>({arrivalPrev[i], timeToPrev[i]}));
    }

    // totalDist = sum(timeToPrev)
    totalDist = engine->makeIntVar(0, 0, MAX_TIME);
    engine->makeInvariant<Linear>(totalDist, timeToPrev);

    VarId leqConst = engine->makeIntVar(100, 100, 100);
    for (int i = 0; i < n; ++i) {
      engine->makeConstraint<LessEqual>(violation[i], arrivalTime[i], leqConst);
    }

    totalViolation = engine->makeIntVar(0, 0, MAX_TIME * n);
    engine->makeInvariant<Linear>(totalViolation, violation);

    engine->close();
    assert(std::all_of(pred.begin(), pred.end(), [&](const VarId p) {
      return engine->lowerBound(p) == 1;
    }));
    assert(std::all_of(pred.begin(), pred.end(), [&](const VarId p) {
      return engine->upperBound(p) == n;
    }));
    assert(std::all_of(pred.begin(), pred.end(), [&](const VarId p) {
      return 1 <= engine->committedValue(p) && engine->committedValue(p) <= n;
    }));

    gen = std::mt19937(rd());

    distribution = std::uniform_int_distribution<Int>{0, n - 1};
  }

  void TearDown(const ::benchmark::State&) override {
    pred.clear();
    timeToPrev.clear();
    arrivalPrev.clear();
    arrivalTime.clear();
    dist.clear();
    violation.clear();
  }
};

inline size_t randInRange(const size_t minVal, const size_t maxVal) {
  return minVal + (std::rand() % (maxVal - minVal + 1));
}

BENCHMARK_DEFINE_F(TSPTW, probe_single_swap)(benchmark::State& st) {
  std::vector<Int> indexVec1(n);
  std::iota(indexVec1.begin(), indexVec1.end(), 0);
  std::vector<Int> indexVec2(n);
  std::iota(indexVec2.begin(), indexVec2.end(), 0);

  for (auto _ : st) {
    Int i = static_cast<Int>(n);
    Int j = static_cast<Int>(n);
    for (size_t index1 = 0; i == n && index1 < static_cast<size_t>(n);
         ++index1) {
      std::swap(indexVec1[index1], indexVec1[randInRange(index1, n - 1)]);
      const Int tmpI = indexVec1[index1];
      for (size_t index2 = 0; index2 < static_cast<size_t>(n); ++index2) {
        std::swap(indexVec2[index2],
                  indexVec2[randInRange(index2, static_cast<size_t>(n - 1))]);
        const Int tmpJ = indexVec2[index2];
        if (tmpI != tmpJ && engine->committedValue(pred[tmpI]) != tmpJ + 1) {
          i = tmpI;
          j = tmpJ;
          break;
        }
      }
    }
    assert(i < static_cast<Int>(n));
    assert(j < static_cast<Int>(n));
    engine->beginMove();
    engine->setValue(
        pred[i],
        engine->committedValue(pred.at(engine->committedValue(pred[i]) - 1)));
    engine->setValue(pred[j], engine->committedValue(pred[i]));
    engine->setValue(pred.at(engine->committedValue(pred[i]) - 1),
                     engine->committedValue(pred[j]));
    engine->endMove();

    engine->beginProbe();
    engine->query(totalDist);
    engine->query(totalViolation);
    engine->endProbe();
  }
}

BENCHMARK_DEFINE_F(TSPTW, probe_all_relocate)(benchmark::State& st) {
  Int probes = 0;
  for (auto _ : st) {
    for (int i = 0; i < n; ++i) {
      for (int j = 0; j < n; ++j) {
        if (i == j || engine->committedValue(pred[i]) == j + 1) {
          continue;
        }
        engine->beginMove();
        engine->setValue(pred[i], engine->committedValue(pred.at(
                                      engine->committedValue(pred[i]) - 1)));
        engine->setValue(pred[j], engine->committedValue(pred[i]));
        engine->setValue(pred.at(engine->committedValue(pred[i]) - 1),
                         engine->committedValue(pred[j]));
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

///*
static void arguments(benchmark::internal::Benchmark* b) {
  for (int i = 10; i <= 150; i += 10) {
    for (int mode = 0; mode <= 3; ++mode) {
      b->Args({i, mode});
    }
#ifndef NDEBUG
    return;
#endif
  }
}

BENCHMARK_REGISTER_F(TSPTW, probe_single_swap)
    ->Apply(arguments)
    ->Unit(benchmark::kMillisecond);

/*
BENCHMARK_REGISTER_F(TSPTW, probe_all_relocate)
    ->Apply(arguments)
    ->Unit(benchmark::kMillisecond);
//*/