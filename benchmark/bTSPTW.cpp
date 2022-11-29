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
#include "invariants/plus.hpp"
#include "views/elementConst.hpp"
#include "views/intOffsetView.hpp"
#include "views/lessEqualConst.hpp"

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
    // First location is the dummy location:
    n = state.range(0) + 1;

    if (n < 3) {
      throw std::runtime_error("There must be at least 3 locations.");
    }

    engine->open();

    setEngineModes(*engine, state.range(1));

    // The first row and column hold dummy values:
    for (int i = 0; i < n; ++i) {
      dist.emplace_back();
      for (int j = 0; j < n; ++j) {
        dist.back().push_back(i * j);
      }
    }

    pred = std::vector<VarId>(n);
    arrivalTime = std::vector<VarId>(n);
    timeToPrev = std::vector<VarId>(n, NULL_ID);
    arrivalPrev = std::vector<VarId>(n, NULL_ID);

    for (Int i = 0; i < n; ++i) {
      const Int initVal = (i - 1 + n) % n;
      pred.at(i) = engine->makeIntVar(initVal, 0 + static_cast<Int>(i == 0),
                                      n - 1 - static_cast<Int>(i == n - 1));
      assert(engine->committedValue(pred.at(i)) == (i - 1 + n) % n);
      assert(engine->currentValue(pred.at(i)) == (i - 1 + n) % n);
    }

    assert(isTourValid(false));
    assert(isTourValid(true));

    // Ignore index 0
    for (int i = 1; i < n; ++i) {
      // timeToPrev[i] = dist[i][pred[i]]
      timeToPrev[i] =
          engine->makeIntView<ElementConst>(*engine, pred[i], dist[i], 0);
      arrivalPrev.at(i) = engine->makeIntVar(0, 0, MAX_TIME);
      arrivalTime.at(i) = engine->makeIntVar(0, 0, MAX_TIME);
    }

    // Ignore index 0
    // Creating n - 1 dynamic invariants, each with 1 static variable and n
    // dynamic variables, resulting in n * n dynamic edges
    // Creating n - 1 static invariants, each with 2 static edges
    arrivalPrev.at(0) = engine->makeIntVar(0, 0, 0);
    arrivalTime.at(0) = arrivalPrev.at(0);
    for (int i = 1; i < n; ++i) {
      // arrivalPrev[i] = arrivalTime[pred[i]]
      engine->makeInvariant<ElementVar>(*engine, arrivalPrev[i], pred[i],
                                        arrivalTime, 0);
      // arrivalTime[i] = arrivalPrev[i] + timeToPrev[i]
      engine->makeInvariant<Plus>(*engine, arrivalTime[i], arrivalPrev[i],
                                  timeToPrev[i]);
    }

    // totalDist = sum(timeToPrev)
    totalDist = engine->makeIntVar(0, 0, MAX_TIME);
    // Remove the first dummy value:
    assert(timeToPrev.front() == NULL_ID);
    timeToPrev.erase(timeToPrev.begin());
    assert(timeToPrev.front() != NULL_ID);
    engine->makeInvariant<Linear>(*engine, totalDist, timeToPrev);

    VarId leqConst = engine->makeIntVar(100, 100, 100);
    violation = std::vector<VarId>(n);
    for (int i = 0; i < n; ++i) {
      violation.at(i) = engine->makeIntVar(0, 0, MAX_TIME);
      engine->makeConstraint<LessEqual>(*engine, violation.at(i),
                                        arrivalTime[i], leqConst);
    }

    totalViolation = engine->makeIntVar(0, 0, MAX_TIME * n);
    engine->makeInvariant<Linear>(*engine, totalViolation, violation);

    engine->close();
    assert(engine->lowerBound(pred.front()) == 1);
    assert(engine->upperBound(pred.back()) == n - 2);
    assert(std::all_of(pred.begin() + 1, pred.end(), [&](const VarId p) {
      return engine->lowerBound(p) == 0;
    }));
    assert(std::all_of(pred.begin(), pred.end() - 1, [&](const VarId p) {
      return engine->upperBound(p) == n - 1;
    }));
    assert(std::all_of(pred.begin(), pred.end(), [&](const VarId p) {
      return 0 <= engine->committedValue(p) && engine->committedValue(p) < n;
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

  bool isTourValid(bool committedValue) const {
    std::vector<bool> visited(n, false);
    Int cur = 0;
    Int numVisited = 0;
    while (!visited.at(cur)) {
      ++numVisited;
      visited.at(cur) = true;
      cur = committedValue ? engine->committedValue(pred.at(cur))
                           : engine->currentValue(pred.at(cur));
      if (cur < 0 || n <= cur) {
        return false;
      }
    }
    return numVisited == n;
  }

  Int computeDistance() {
    Int tot = 0;
    for (Int i = 0; i < n; ++i) {
      tot += dist.at(i).at(engine->currentValue(pred.at(i)));
    }
    return tot;
  }
};

inline size_t randInRange(const size_t minVal, const size_t maxVal) {
  return minVal + (std::rand() % (maxVal - minVal + 1));
}

BENCHMARK_DEFINE_F(TSPTW, probe_three_opt)(benchmark::State& st) {
  std::vector<Int> indexVec1(n);
  std::iota(indexVec1.begin(), indexVec1.end(), 0);
  std::vector<Int> indexVec2(n);
  std::iota(indexVec2.begin(), indexVec2.end(), 0);

  size_t probes = 0;
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
        if (tmpI != tmpJ && engine->committedValue(pred[tmpI]) != tmpJ) {
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
        pred[i], engine->committedValue(pred[engine->committedValue(pred[i])]));
    engine->setValue(pred[j], engine->committedValue(pred[i]));
    engine->setValue(pred[engine->committedValue(pred[i])],
                     engine->committedValue(pred[j]));
    engine->endMove();

    engine->beginProbe();
    engine->query(totalDist);
    engine->query(totalViolation);
    engine->endProbe();
    ++probes;
    assert(isTourValid(true));
    assert(isTourValid(false));
  }
  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
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
        engine->setValue(
            pred[i],
            engine->committedValue(pred[engine->committedValue(pred[i]) - 1]));
        engine->setValue(pred[j], engine->committedValue(pred[i]));
        engine->setValue(pred[engine->committedValue(pred[i]) - 1],
                         engine->committedValue(pred[j]));
        engine->endMove();

        engine->beginProbe();
        engine->query(totalDist);
        engine->query(totalViolation);
        engine->endProbe();
        assert(engine->currentValue(totalDist) == computeDistance());
        ++probes;
        assert(isTourValid(true));
        assert(isTourValid(false));
      }
    }
  }
  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
}

//*
BENCHMARK_REGISTER_F(TSPTW, probe_three_opt)
    ->Unit(benchmark::kMillisecond)
    ->Apply(defaultArguments);

/*
BENCHMARK_REGISTER_F(TSPTW, probe_all_relocate)
    ->Unit(benchmark::kMillisecond)
    ->Apply(defaultArguments);
//*/