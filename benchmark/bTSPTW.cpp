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
#include "atlantis/propagation/views/intMaxView.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"
#include "atlantis/propagation/views/lessEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/lessEqual.hpp"
#include "benchmark.hpp"

namespace atlantis::benchmark {

class TSPTW : public ::benchmark::Fixture {
 public:
  std::shared_ptr<propagation::Solver> solver;
  std::vector<std::vector<Int>> durations;

  std::vector<propagation::VarViewId> pred;
  std::vector<propagation::VarViewId> timeToPred;
  std::vector<propagation::VarViewId> arrivalTime;
  std::vector<propagation::VarViewId> departureTime;
  std::vector<propagation::VarViewId> departureTimePred;
  std::vector<propagation::VarViewId> violations;

  propagation::VarViewId objective{propagation::NULL_ID};

  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<Int> distribution;
  Int n{0};
  const int MAX_TIME = 100000;

  propagation::VarViewId totalViolation{propagation::NULL_ID};

  void SetUp(const ::benchmark::State& state) override {
    solver = std::make_shared<propagation::Solver>();
    // First location is the dummy location:
    n = state.range(0) + 1;

    if (n < 3) {
      throw std::runtime_error("There must be at least 3 locations.");
    }

    solver->open();

    setSolverMode(*solver, static_cast<int>(state.range(1)));

    durations.clear();
    pred.clear();
    timeToPred.clear();
    arrivalTime.clear();
    departureTime.clear();
    departureTimePred.clear();
    violations.clear();

    for (Int i = 0; i < n; ++i) {
      durations.emplace_back(n + 1, 0);
      for (int j = 0; j < n; ++j) {
        durations.back().at(j) = (i + 1) * (j + 1);
      }
    }
    // add dummy row:
    durations.emplace_back(n + 1, 0);

    pred.reserve(n);
    arrivalTime.reserve(n);
    timeToPred.reserve(n);
    departureTimePred.reserve(n);
    violations.reserve(n);

    for (Int i = 0; i < n; ++i) {
      pred.emplace_back(solver->makeIntVar(i + 1, i == 0 ? 1 : 0, n));
      assert(solver->committedValue(pred.at(i)) == i + 1);
      assert(solver->currentValue(pred.at(i)) == i + 1);
    }
    // add dummy location
    pred.emplace_back(solver->makeIntVar(0, 0, n - 1));

    assert(isTourValid(false));
    assert(isTourValid(true));

    std::vector<Int> earliestVisitingTime(n, 100);
    std::vector<Int> latestVisitingTime(n, 200);

    // Create the variables:
    for (Int i = 0; i < n; ++i) {
      // timeToPred[i] = durations[i][pred[i]]
      timeToPred.emplace_back(solver->makeIntView<propagation::ElementConst>(
          *solver, pred[i], std::vector<Int>(durations[i]), 0));
      arrivalTime.emplace_back(solver->makeIntVar(0, 0, MAX_TIME));
      // departureTime[i] = max(arrivalTime[i], latestVisitingTime[i])
      departureTime.emplace_back(solver->makeIntView<propagation::IntMaxView>(
          *solver, arrivalTime[i], earliestVisitingTime[i]));
      departureTimePred.emplace_back(solver->makeIntVar(0, 0, MAX_TIME));
      // violations[i] = max(0, arrivalTime[i] - latestVisitingTime[i])
      violations.emplace_back(solver->makeIntView<propagation::LessEqualConst>(
          *solver, arrivalTime[i], latestVisitingTime[i]));
    }

    // create additional variable for the dummy location n + 1:
    departureTime.emplace_back(solver->makeIntVar(0, 0, 0));

    for (Int i = 0; i < n; ++i) {
      // departureTimePred[i] = departureTime[pred[i]]
      solver->makeInvariant<propagation::ElementVar>(
          *solver, departureTimePred[i], pred[i],
          std::vector<propagation::VarViewId>(departureTime), 0);
    }

    for (Int i = 0; i < n; ++i) {
      // arrivalTime[i] = departureTimePred[i] + timeToPred[i]
      solver->makeInvariant<propagation::Plus>(
          *solver, arrivalTime[i], departureTimePred[i], timeToPred[i]);
    }

    // The objective is the departure time of the location preceding the dummy
    // location departureTimePred[pred[n]]
    objective = solver->makeIntVar(0, 0, MAX_TIME * n);
    solver->makeInvariant<propagation::ElementVar>(
        *solver, objective, pred[n],
        std::vector<propagation::VarViewId>(departureTime), 0);

    totalViolation = solver->makeIntVar(0, 0, MAX_TIME * n);
    solver->makeInvariant<propagation::Linear>(
        *solver, totalViolation,
        std::vector<propagation::VarViewId>(violations));

    solver->close();
    assert(solver->lowerBound(pred.front()) == 1);
    assert(solver->upperBound(pred.back()) == n - 1);
    assert(std::all_of(pred.begin() + 1, pred.end(),
                       [&](const propagation::VarViewId p) {
                         return solver->lowerBound(p) == 0;
                       }));
    assert(std::all_of(pred.begin(), pred.end() - 1,
                       [&](const propagation::VarViewId p) {
                         return solver->upperBound(p) == n;
                       }));
    assert(std::all_of(pred.begin(), pred.end(),
                       [&](const propagation::VarViewId p) {
                         return 0 <= solver->committedValue(p) &&
                                solver->committedValue(p) <= n;
                       }));

    gen = std::mt19937(rd());

    distribution = std::uniform_int_distribution<Int>{0, n - 1};
  }

  void TearDown(const ::benchmark::State&) override {
    durations.clear();
    pred.clear();
    timeToPred.clear();
    arrivalTime.clear();
    departureTime.clear();
    departureTimePred.clear();
    violations.clear();
  }

  [[nodiscard]] bool isTourValid(bool committedValue) const {
    std::vector<bool> visited(n + 1, false);
    // n is the dummy location
    Int cur = n;
    Int numVisited = 0;
    while (!visited.at(cur)) {
      ++numVisited;
      visited.at(cur) = true;
      cur = committedValue ? solver->committedValue(pred.at(cur))
                           : solver->currentValue(pred.at(cur));
      if (cur < 0 || n + 1 <= cur) {
        return false;
      }
    }
    return numVisited == n + 1;
  }

  Int computeDistance() {
    Int tot = 0;
    for (Int i = 0; i < n + 1; ++i) {
      tot += durations.at(i).at(solver->currentValue(pred.at(i)));
    }
    return tot;
  }
};

inline size_t randInRange(const size_t minVal, const size_t maxVal) {
  return minVal + (std::rand() % (maxVal - minVal + 1));
}

BENCHMARK_DEFINE_F(TSPTW, probe_three_opt)(::benchmark::State& st) {
  std::vector<Int> indexVec1(n + 1);
  std::iota(indexVec1.begin(), indexVec1.end(), 0);
  std::vector<Int> indexVec2(n + 1);
  std::iota(indexVec2.begin(), indexVec2.end(), 0);

  size_t probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    const Int i = randInRange(0, n);
    Int tmp = randInRange(0, n - 2);
    tmp = (tmp == i || tmp == solver->committedValue(pred[i])) ? (n - 1) : tmp;
    tmp = (tmp == i || tmp == solver->committedValue(pred[i])) ? n : tmp;
    assert(tmp != i && tmp != solver->committedValue(pred[i]));
    const Int j = tmp;

    assert(0 <= i && i <= n);
    assert(0 <= j && j <= n);
    solver->beginMove();
    solver->setValue(
        pred[i], solver->committedValue(pred[solver->committedValue(pred[i])]));
    solver->setValue(pred[j], solver->committedValue(pred[i]));
    solver->setValue(pred[solver->committedValue(pred[i])],
                     solver->committedValue(pred[j]));
    solver->endMove();

    solver->beginProbe();
    solver->query(objective);
    solver->query(totalViolation);
    solver->endProbe();
    ++probes;
    assert(isTourValid(true));
    assert(isTourValid(false));
  }
  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(TSPTW, probe_all_relocate)(::benchmark::State& st) {
  Int probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    for (Int i = 0; i <= n; ++i) {
      for (int j = 0; j <= n; ++j) {
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
        solver->query(objective);
        solver->query(totalViolation);
        solver->endProbe();
        assert(solver->currentValue(objective) == computeDistance());
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
BENCHMARK_REGISTER_F(TSPTW, probe_three_opt)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments);

/*
BENCHMARK_REGISTER_F(TSPTW, probe_all_relocate)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments);
//*/
}  // namespace atlantis::benchmark
