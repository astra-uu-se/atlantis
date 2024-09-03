#include <benchmark/benchmark.h>

#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"
#include "benchmark.hpp"

namespace atlantis::benchmark {

class Queens : public ::benchmark::Fixture {
 public:
  std::shared_ptr<propagation::Solver> solver;
  std::vector<propagation::VarId> queens;
  std::vector<propagation::VarId> q_offset_plus;
  std::vector<propagation::VarId> q_offset_minus;
  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<Int> distribution;
  Int n;

  propagation::VarId violation1 = propagation::NULL_ID;
  propagation::VarId violation2 = propagation::NULL_ID;
  propagation::VarId violation3 = propagation::NULL_ID;
  propagation::VarId totalViolation = propagation::NULL_ID;

  void SetUp(const ::benchmark::State& state) override {
    solver = std::make_shared<propagation::Solver>();
    n = state.range(0);
    if (n < 0) {
      throw std::runtime_error("n must be non-negative.");
    }

    solver->open();
    setSolverMode(*solver, static_cast<int>(state.range(1)));
    // the total number of variables is linear in n
    queens = std::vector<propagation::VarId>(n);
    q_offset_minus = std::vector<propagation::VarId>(n);
    q_offset_plus = std::vector<propagation::VarId>(n);

    for (Int i = 0; i < n; ++i) {
      queens.at(i) = solver->makeIntVar(i, 0, n - 1);
      q_offset_minus.at(i) = solver->makeIntView<propagation::IntOffsetView>(
          *solver, queens.at(i), -i);
      q_offset_plus.at(i) = solver->makeIntView<propagation::IntOffsetView>(
          *solver, queens.at(i), i);
    }

    violation1 = solver->makeIntVar(0, 0, n);
    violation2 = solver->makeIntVar(0, 0, n);
    violation3 = solver->makeIntVar(0, 0, n);

    // 3 invariants, each having taking n static input variables
    solver->makeViolationInvariant<propagation::AllDifferent>(
        *solver, violation1, std::vector<propagation::VarId>(queens));
    solver->makeViolationInvariant<propagation::AllDifferent>(
        *solver, violation2, std::vector<propagation::VarId>(q_offset_minus));
    solver->makeViolationInvariant<propagation::AllDifferent>(
        *solver, violation3, std::vector<propagation::VarId>(q_offset_plus));

    totalViolation = solver->makeIntVar(0, 0, 3 * n);

    solver->makeInvariant<propagation::Linear>(
        *solver, totalViolation,
        std::vector<propagation::VarId>{violation1, violation2, violation3});

    solver->close();

    gen = std::mt19937(rd());

    distribution = std::uniform_int_distribution<Int>{0, n - 1};
  }

  void TearDown(const ::benchmark::State&) override {
    queens.clear();
    q_offset_minus.clear();
    q_offset_plus.clear();
  }

  std::string instanceToString() {
    std::string str = "Queens: ";
    for (auto q : queens) {
      str += std::to_string(solver->committedValue(q)) + ", ";
    }
    return str;
  }

  inline bool sanity() const {
    return all_in_range(0, n - 1, [&](const size_t i) {
      return all_in_range(i + 1, n, [&](const size_t j) {
        return solver->committedValue(queens.at(i)) !=
                   solver->committedValue(queens.at(j)) &&
               solver->currentValue(queens.at(i)) !=
                   solver->currentValue(queens.at(j));
      });
    });
  }
};

BENCHMARK_DEFINE_F(Queens, probe_single_swap)(::benchmark::State& st) {
  size_t probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    const size_t i = distribution(gen);
    assert(i < queens.size());
    const size_t j = distribution(gen);
    assert(j < queens.size());
    const Int oldI = solver->committedValue(queens[i]);
    const Int oldJ = solver->committedValue(queens[j]);
    // Perform random swap
    solver->beginMove();
    solver->setValue(queens[i], oldJ);
    solver->setValue(queens[j], oldI);
    solver->endMove();

    solver->beginProbe();
    solver->query(totalViolation);
    solver->endProbe();
    ++probes;
    assert(sanity());
  }
  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(Queens, probe_all_swap)(::benchmark::State& st) {
  int probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    for (size_t i = 0; i < static_cast<size_t>(n); ++i) {
      for (size_t j = i + 1; j < static_cast<size_t>(n); ++j) {
        const Int oldI = solver->committedValue(queens[i]);
        const Int oldJ = solver->committedValue(queens[j]);
        solver->beginMove();
        solver->setValue(queens[i], oldJ);
        solver->setValue(queens[j], oldI);
        solver->endMove();

        solver->beginProbe();
        solver->query(totalViolation);
        solver->endProbe();

        ++probes;
        assert(sanity());
      }
    }
  }
  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(Queens, solve)(::benchmark::State& st) {
  Int it = 0;
  Int probes = 0;

  std::vector<Int> tabu;
  tabu.assign(n, 0);
  const Int tenure = 10;
  bool done = false;

  for ([[maybe_unused]] const auto& _ : st) {
    while (it < 100000 && !done) {
      size_t bestI = 0;
      size_t bestJ = 0;
      Int bestViol = n;
      for (size_t i = 0; i < static_cast<size_t>(n); ++i) {
        for (size_t j = i + 1; j < static_cast<size_t>(n); ++j) {
          if (tabu[i] > it && tabu[j] > it) {
            continue;
          }
          const Int oldI = solver->committedValue(queens[i]);
          const Int oldJ = solver->committedValue(queens[j]);

          solver->beginMove();
          solver->setValue(queens[i], oldJ);
          solver->setValue(queens[j], oldI);
          solver->endMove();

          solver->beginProbe();
          solver->query(totalViolation);
          solver->endProbe();

          ++probes;
          assert(sanity());

          Int newValue = solver->currentValue(totalViolation);
          if (newValue <= bestViol) {
            bestViol = newValue;
            bestI = i;
            bestJ = j;
          }
        }
      }

      const Int oldI = solver->committedValue(queens[bestI]);
      const Int oldJ = solver->committedValue(queens[bestJ]);
      solver->beginMove();
      solver->setValue(queens[bestI], oldJ);
      solver->setValue(queens[bestJ], oldI);
      solver->endMove();

      solver->beginCommit();
      solver->query(totalViolation);
      solver->endCommit();

      tabu[bestI] = it + tenure;
      tabu[bestJ] = it + tenure;
      if (bestViol == 0) {
        done = true;
      }
      ++it;
    }
  }

  st.counters["it"] = ::benchmark::Counter(it);

  st.counters["it_per_s"] =
      ::benchmark::Counter(it, ::benchmark::Counter::kIsRate);
  st.counters["probes_per_s"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
  st.counters["solved"] = ::benchmark::Counter(done);
  logDebug(instanceToString());
}

//*
BENCHMARK_REGISTER_F(Queens, probe_single_swap)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments);

/*
BENCHMARK_REGISTER_F(Queens, probe_all_swap)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments);

/*
BENCHMARK_REGISTER_F(Queens, solve)->Apply(defaultArguments);
//*/
}  // namespace atlantis::benchmark
