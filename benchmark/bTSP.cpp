#include <benchmark/benchmark.h>

#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "atlantis/propagation/invariants/elementVar.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/invariants/plus.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/elementConst.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"
#include "atlantis/propagation/views/lessEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/lessEqual.hpp"
#include "benchmark.hpp"

namespace atlantis::benchmark {

class TSP : public ::benchmark::Fixture {
 public:
  std::unique_ptr<propagation::Solver> solver;
  std::vector<propagation::VarId> pred;
  std::vector<propagation::VarId> timeToPred;
  std::vector<std::vector<Int>> durations;
  propagation::VarId totalDist;

  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<Int> distribution;
  Int n{0};
  const int MAX_TIME = 100000;

  void SetUp(const ::benchmark::State& state) override {
    solver = std::make_unique<propagation::Solver>();
    n = state.range(0);

    if (n < 3) {
      throw std::runtime_error("There must be at least 3 locations.");
    }

    solver->open();

    setSolverMode(*solver, static_cast<int>(state.range(1)));

    for (Int i = 0; i < n; ++i) {
      durations.emplace_back();
      for (Int j = 0; j < n; ++j) {
        durations.back().push_back(i * j);
      }
    }

    pred = std::vector<propagation::VarId>(n, propagation::NULL_ID);
    timeToPred = std::vector<propagation::VarId>(n, propagation::NULL_ID);

    for (Int i = 0; i < n; ++i) {
      const Int initVal = (i - 1 + n) % n;
      pred[i] = solver->makeIntVar(initVal, 0 + static_cast<Int>(i == 0),
                                   n - 1 - static_cast<Int>(i == n - 1));
      assert(solver->committedValue(pred.at(i)) == (i - 1 + n) % n);
      assert(solver->currentValue(pred.at(i)) == (i - 1 + n) % n);
    }

    assert(isTourValid(false));
    assert(isTourValid(true));

    for (int i = 0; i < n; ++i) {
      // timeToPred[i] = durations[i][pred[i]]
      timeToPred[i] = solver->makeIntView<propagation::ElementConst>(
          *solver, pred[i], std::vector<Int>(durations[i]), 0);
    }

    // totalDist = sum(timeToPred)
    totalDist = solver->makeIntVar(0, 0, MAX_TIME);
    solver->makeInvariant<propagation::Linear>(
        *solver, totalDist, std::vector<propagation::VarId>(timeToPred));

    solver->close();
    assert(
        std::all_of(pred.begin(), pred.end(), [&](const propagation::VarId p) {
          return solver->lowerBound(p) == 0;
        }));
    assert(
        std::all_of(pred.begin(), pred.end(), [&](const propagation::VarId p) {
          return solver->upperBound(p) == n;
        }));
    assert(
        std::all_of(pred.begin(), pred.end(), [&](const propagation::VarId p) {
          return 0 <= solver->committedValue(p) &&
                 solver->committedValue(p) < n;
        }));

    gen = std::mt19937(rd());

    distribution = std::uniform_int_distribution<Int>{0, n - 1};
  }

  void TearDown(const ::benchmark::State&) override {
    pred.clear();
    timeToPred.clear();
  }

  [[nodiscard]] bool isTourValid(bool committedValue) const {
    std::vector<bool> visited(n, false);
    Int cur = 0;
    Int numVisited = 0;
    while (!visited.at(cur)) {
      ++numVisited;
      visited.at(cur) = true;
      cur = committedValue ? solver->committedValue(pred.at(cur))
                           : solver->currentValue(pred.at(cur));
      if (cur < 0 || n <= cur) {
        return false;
      }
    }
    return numVisited == n;
  }

  Int computeDistance() {
    Int tot = 0;
    for (Int i = 0; i < n; ++i) {
      tot += durations.at(i).at(solver->currentValue(pred.at(i)));
    }
    return tot;
  }
};

inline size_t randInRange(const size_t minVal, const size_t maxVal) {
  return minVal + (std::rand() % (maxVal - minVal + 1));
}

BENCHMARK_DEFINE_F(TSP, probe_three_opt)(::benchmark::State& st) {
  std::vector<Int> indexVec1(n);
  std::iota(indexVec1.begin(), indexVec1.end(), 0);
  std::vector<Int> indexVec2(n);
  std::iota(indexVec2.begin(), indexVec2.end(), 0);

  size_t probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
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
        if (tmpI != tmpJ && solver->committedValue(pred[tmpI]) != tmpJ) {
          i = tmpI;
          j = tmpJ;
          break;
        }
      }
    }
    assert(i < static_cast<Int>(n));
    assert(j < static_cast<Int>(n));
    solver->beginMove();
    solver->setValue(
        pred[i], solver->committedValue(pred[solver->committedValue(pred[i])]));
    solver->setValue(pred[j], solver->committedValue(pred[i]));
    solver->setValue(pred[solver->committedValue(pred[i])],
                     solver->committedValue(pred[j]));
    solver->endMove();

    solver->beginProbe();
    solver->query(totalDist);
    solver->endProbe();
    ++probes;
    assert(isTourValid(true));
    assert(isTourValid(false));
  }
  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(TSP, probe_all_relocate)(::benchmark::State& st) {
  Int probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    for (int i = 0; i < n; ++i) {
      for (int j = 0; j < n; ++j) {
        if (i == j || solver->committedValue(pred[i]) == j + 1) {
          continue;
        }
        solver->beginMove();
        solver->setValue(
            pred[i],
            solver->committedValue(pred[solver->committedValue(pred[i]) - 1]));
        solver->setValue(pred[j], solver->committedValue(pred[i]));
        solver->setValue(pred[solver->committedValue(pred[i]) - 1],
                         solver->committedValue(pred[j]));
        solver->endMove();

        solver->beginProbe();
        solver->query(totalDist);
        solver->endProbe();
        assert(solver->currentValue(totalDist) == computeDistance());
        ++probes;
        assert(isTourValid(true));
        assert(isTourValid(false));
      }
    }
  }
  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

//*
BENCHMARK_REGISTER_F(TSP, probe_three_opt)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments);

/*
BENCHMARK_REGISTER_F(TSP, probe_all_relocate)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments);
//*/
}  // namespace atlantis::benchmark
