#include <benchmark/benchmark.h>

#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "atlantis/propagation/invariants/binaryMax.hpp"
#include "atlantis/propagation/invariants/element2dConst.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/invariants/plus.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/elementConst.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"
#include "atlantis/propagation/views/lessEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/lessEqual.hpp"
#include "benchmark.hpp"

namespace atlantis::benchmark {

class TSPTWAllDiff : public ::benchmark::Fixture {
 public:
  std::shared_ptr<propagation::Solver> solver;
  std::vector<propagation::VarViewId> sequence;
  std::vector<propagation::VarViewId> timeTo;
  std::vector<propagation::VarViewId> arrivalTime;
  std::vector<propagation::VarViewId> earliestVisitingTime;
  std::vector<propagation::VarViewId> latestVisitingTime;
  std::vector<propagation::VarViewId> departureTime;
  std::vector<std::vector<Int>> dist;
  std::vector<Int> earliestVisit;
  std::vector<Int> latestVisit;
  propagation::VarViewId totalDist{propagation::NULL_ID};

  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<Int> distribution;
  Int n{0};
  const int MAX_TIME = 100000;

  std::vector<propagation::VarViewId> violations;
  VarViewId totalViolation{propagation::NULL_ID};

  void SetUp(const ::benchmark::State& state) override {
    solver = std::make_shared<propagation::Solver>();
    // First location is the dummy location:
    n = state.range(0);

    if (n < 3) {
      throw std::runtime_error("There must be at least 3 locations.");
    }

    solver->open();

    setSolverMode(*solver, static_cast<int>(state.range(1)));
    dist.clear();

    for (Int i = 1; i <= n; ++i) {
      dist.emplace_back();
      for (int j = 1; j <= n; ++j) {
        dist.back().push_back(i * j);
      }
    }

    sequence.clear();
    timeTo.clear();
    arrivalTime.clear();
    earliestVisitingTime.clear();
    latestVisitingTime.clear();
    departureTime.clear();
    earliestVisit.clear();
    latestVisit.clear();

    sequence.emplace_back(solver->makeIntVar(0, 0, n - 1));
    timeTo.emplace_back(propagation::NULL_ID);
    arrivalTime.emplace_back(solver->makeIntVar(0, 0, 0));
    departureTime.emplace_back(propagation::NULL_ID);

    for (Int i = 1; i < n; ++i) {
      sequence.emplace_back(solver->makeIntVar(i, 0, n - 1));
      timeTo.emplace_back(solver->makeIntVar(0, 0, MAX_TIME));
      arrivalTime.emplace_back(solver->makeIntVar(0, 0, MAX_TIME));
      departureTime.emplace_back(solver->makeIntVar(0, 0, MAX_TIME));
    }

    assert(all_in_range(0, n - 1, [&](const size_t a) {
      const Int curA = solver->currentValue(sequence.at(a));
      return all_in_range(a + 1, n, [&](const size_t b) {
        const Int curB = solver->currentValue(sequence.at(b));
        if (curA == curB) {
          return false;
        }
        return curA != curB;
      });
    }));

    earliestVisit.resize(n, 100);
    latestVisit.resize(n, 200);

    earliestVisitingTime.reserve(n);
    latestVisitingTime.reserve(n);
    for (Int i = 0; i < n; ++i) {
      earliestVisitingTime.emplace_back(
          solver->makeIntView<propagation::ElementConst>(
              *solver, sequence[i], std::vector<Int>(earliestVisit), 0));
      latestVisitingTime.emplace_back(
          solver->makeIntView<propagation::ElementConst>(
              *solver, sequence[i], std::vector<Int>(latestVisit), 0));
    }

    assert(departureTime[0] == propagation::NULL_ID);

    departureTime[0] = earliestVisitingTime[0];

    // Ignore index 0
    for (Int i = 1; i < n; ++i) {
      // timeTo[i] = dist[sequence[i - 1]][sequence[i]]
      solver->makeInvariant<propagation::Element2dConst>(
          *solver, timeTo[i], sequence[i - 1], sequence[i],
          std::vector<std::vector<Int>>(dist), 0, 0);

      // arrivalTime[i] = departureTime[i - 1] + timeTo[i];
      solver->makeInvariant<propagation::Plus>(*solver, arrivalTime[i],
                                               departureTime[i - 1], timeTo[i]);

      // departureTime[i] = max(arrivalTime[i], earliestVisitingTime[i])
      solver->makeInvariant<propagation::BinaryMax>(
          *solver, departureTime[i], arrivalTime[i], earliestVisitingTime[i]);
    }

    violations.reserve(n - 1);

    for (Int i = 1; i < n; ++i) {
      violations.emplace_back(solver->makeIntVar(0, 0, MAX_TIME));
      // violations[i - 1] = arrivalTime[i] <= latestVisitingTime[i];
      solver->makeViolationInvariant<propagation::LessEqual>(
          *solver, violations.back(), arrivalTime[i], latestVisitingTime[i]);
    }

    totalViolation = solver->makeIntVar(0, 0, MAX_TIME * n);
    solver->makeInvariant<propagation::Linear>(
        *solver, totalViolation,
        std::vector<propagation::VarViewId>(violations));

    totalDist = departureTime.back();

    solver->close();
    assert(computeDistance() > 0);
    assert(std::all_of(sequence.begin(), sequence.end(),
                       [&](const propagation::VarViewId p) {
                         return solver->lowerBound(p) == 0;
                       }));
    assert(std::all_of(sequence.begin(), sequence.end(),
                       [&](const propagation::VarViewId p) {
                         return solver->upperBound(p) == n - 1;
                       }));
    assert(std::all_of(sequence.begin(), sequence.end(),
                       [&](const propagation::VarViewId p) {
                         return 0 <= solver->committedValue(p) &&
                                solver->committedValue(p) <= n - 1;
                       }));

    gen = std::mt19937(rd());

    distribution = std::uniform_int_distribution<Int>{0, n - 1};
  }

  void TearDown(const ::benchmark::State&) override {
    dist.clear();
    sequence.clear();
    timeTo.clear();
    arrivalTime.clear();
    earliestVisitingTime.clear();
    latestVisitingTime.clear();
    departureTime.clear();
    earliestVisit.clear();
    latestVisit.clear();
  }

  Int computeDistance() {
    std::vector<Int> departure(n, 0);
    departure.at(0) = std::max(
        earliestVisit.at(solver->currentValue(sequence.at(0))), Int{0});
    assert(departure.at(0) == solver->currentValue(departureTime.at(0)));

    for (Int i = 1; i < n; ++i) {
      const Int pred = solver->currentValue(sequence.at(i - 1));
      const Int cur = solver->currentValue(sequence.at(i));

      const Int travelTime = dist.at(pred).at(cur);
      assert(travelTime == solver->currentValue(timeTo.at(i)));

      const Int arrival = departure.at(i - 1) + travelTime;
      assert(arrival == solver->currentValue(arrivalTime.at(i)));

      departure.at(i) = std::max(earliestVisit.at(cur), arrival);
      assert(departure.at(i) == solver->currentValue(departureTime.at(i)));
    }
    return departure.back();
  }
};

BENCHMARK_DEFINE_F(TSPTWAllDiff, probe_three_opt)(::benchmark::State& st) {
  size_t probes = 0;
  assert(all_in_range(0, n, [&](const size_t a) {
    const Int curA = solver->committedValue(sequence.at(a));
    return all_in_range(a + 1, n, [&](const size_t b) {
      const Int curB = solver->committedValue(sequence.at(b));
      if (curA == curB) {
        return false;
      }
      return curA != curB;
    });
  }));

  for ([[maybe_unused]] const auto& _ : st) {
    const size_t b = rand_in_range(0, n - 2, gen);
    const size_t d = rand_in_range(b + 1, n - 1, gen);
    const size_t e = rand_in_range(d, n - 1, gen);

    assert(b < d);
    assert(d <= e);
    assert(e < static_cast<size_t>(n));

    solver->beginMove();
    size_t cur = b;

    for (size_t i = d; i <= e; ++i) {
      solver->setValue(sequence[cur++], solver->committedValue(sequence[i]));
    }
    for (size_t i = b; i < d; ++i) {
      solver->setValue(sequence[cur++], solver->committedValue(sequence[i]));
    }

    assert(all_in_range(0, n, [&](const size_t i) {
      const Int curI = solver->currentValue(sequence.at(i));
      return all_in_range(i + 1, n, [&](const size_t j) {
        const Int curJ = solver->currentValue(sequence.at(j));
        if (curI == curJ) {
          return false;
        }
        return curI != curJ;
      });
    }));
    solver->endMove();

    solver->beginProbe();
    solver->query(totalDist);
    solver->query(totalViolation);
    solver->endProbe();

    assert(solver->currentValue(totalDist) == computeDistance());
    ++probes;
  }
  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(TSPTWAllDiff, probe_swap)(::benchmark::State& st) {
  size_t probes = 0;
  assert(all_in_range(0, n, [&](const size_t a) {
    const Int curA = solver->committedValue(sequence.at(a));
    return all_in_range(a + 1, n, [&](const size_t b) {
      const Int curB = solver->committedValue(sequence.at(b));
      if (curA == curB) {
        return false;
      }
      return curA != curB;
    });
  }));

  for ([[maybe_unused]] const auto& _ : st) {
    const size_t a = rand_in_range(0, n - 1, gen);
    const size_t temp = rand_in_range(0, n - 2, gen);
    const size_t b = temp == a ? n - 1 : temp;

    solver->beginMove();
    solver->setValue(sequence[a], solver->committedValue(sequence[b]));
    solver->setValue(sequence[b], solver->committedValue(sequence[a]));

    assert(all_in_range(0, n, [&](const size_t i) {
      const Int curI = solver->currentValue(sequence.at(i));
      return all_in_range(i + 1, n, [&](const size_t j) {
        const Int curJ = solver->currentValue(sequence.at(j));
        if (curI == curJ) {
          return false;
        }
        return curI != curJ;
      });
    }));
    solver->endMove();

    solver->beginProbe();
    solver->query(totalDist);
    solver->query(totalViolation);
    solver->endProbe();

    assert(solver->currentValue(totalDist) == computeDistance());
    ++probes;
  }
  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(TSPTWAllDiff, probe_all_relocate)(::benchmark::State& st) {
  size_t probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    for (Int i = 0; i < n; ++i) {
      for (int j = i + 1; j < n; ++j) {
        solver->beginMove();
        solver->setValue(sequence[i], solver->committedValue(sequence[j]));
        solver->setValue(sequence[j], solver->committedValue(sequence[i]));
        solver->endMove();

        solver->beginProbe();
        solver->query(totalDist);
        solver->query(totalViolation);
        solver->endProbe();
        ++probes;
      }
    }
  }
  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

//*
BENCHMARK_REGISTER_F(TSPTWAllDiff, probe_swap)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments);

/*
BENCHMARK_REGISTER_F(TSPTWAllDiff, probe_all_relocate)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments);
//*/
}  // namespace atlantis::benchmark
