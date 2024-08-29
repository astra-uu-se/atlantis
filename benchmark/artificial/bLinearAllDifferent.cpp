#include <benchmark/benchmark.h>

#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "../benchmark.hpp"
#include "atlantis/misc/logging.hpp"
#include "atlantis/propagation/invariants/absDiff.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"

namespace atlantis::benchmark {

class LinearAllDifferent : public ::benchmark::Fixture {
 public:
  std::unique_ptr<propagation::Solver> solver;
  std::vector<propagation::VarId> decisionVars;
  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<size_t> decionVarIndexDist;
  size_t varCount{0};

  propagation::VarId violation = propagation::NULL_ID;

  void SetUp(const ::benchmark::State& state) override {
    solver = std::make_unique<propagation::Solver>();
    bool overlappingLinears = state.range(0) != 0;
    std::vector<propagation::VarId> linearOutputVars;
    size_t increment;

    if (overlappingLinears) {
      varCount = state.range(1);
      linearOutputVars.reserve(varCount - 1);
      increment = 1;
    } else {
      varCount = state.range(1) - (state.range(1) % 2);
      linearOutputVars.reserve(varCount / 2);
      increment = 2;
    }

    solver->open();
    setSolverMode(*solver, static_cast<int>(state.range(2)));

    decisionVars.reserve(varCount);

    for (size_t i = 0; i < varCount; ++i) {
      decisionVars.push_back(solver->makeIntVar(
          static_cast<Int>(i), 0, static_cast<Int>(varCount) - 1));
    }

    for (size_t i = 0; i < varCount - 1; i += increment) {
      linearOutputVars.push_back(solver->makeIntVar(
          static_cast<Int>(i), 0, 2 * (static_cast<Int>(varCount) - 1)));
      solver->makeInvariant<propagation::Linear>(
          *solver, linearOutputVars.back(),
          std::vector<propagation::VarId>{decisionVars.at(i),
                                          decisionVars.at(i + 1)});
    }

    violation = solver->makeIntVar(0, 0, static_cast<Int>(varCount));
    solver->makeViolationInvariant<propagation::AllDifferent>(
        *solver, violation, std::move(linearOutputVars));

    solver->close();

    gen = std::mt19937(rd());

    decionVarIndexDist = std::uniform_int_distribution<size_t>{0, varCount - 1};
  }

  void TearDown(const ::benchmark::State&) override { decisionVars.clear(); }
};

BENCHMARK_DEFINE_F(LinearAllDifferent, probe_single_swap)
(::benchmark::State& st) {
  Int probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    size_t i = decionVarIndexDist(gen);
    size_t j = decionVarIndexDist(gen);

    // Perform random swap
    solver->beginMove();
    solver->setValue(decisionVars.at(i),
                     solver->committedValue(decisionVars.at(j)));
    solver->setValue(decisionVars.at(j),
                     solver->committedValue(decisionVars.at(i)));
    solver->endMove();

    solver->beginProbe();
    solver->query(violation);
    solver->endProbe();

    ++probes;
  }

  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(LinearAllDifferent, probe_all_swap)
(::benchmark::State& st) {
  size_t probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    for (size_t i = 0; i < varCount; ++i) {
      for (size_t j = i + 1; j < varCount; ++j) {
        solver->beginMove();
        solver->setValue(decisionVars.at(i),
                         solver->committedValue(decisionVars.at(j)));
        solver->setValue(decisionVars.at(j),
                         solver->committedValue(decisionVars.at(i)));
        solver->endMove();

        solver->beginProbe();
        solver->query(violation);
        solver->endProbe();

        ++probes;
      }
    }
  }
  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(LinearAllDifferent, commit_single_swap)
(::benchmark::State& st) {
  size_t commits = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    size_t i = decionVarIndexDist(gen);
    size_t j = decionVarIndexDist(gen);

    // Perform random swap
    solver->beginMove();
    solver->setValue(decisionVars.at(i),
                     solver->committedValue(decisionVars.at(j)));
    solver->setValue(decisionVars.at(j),
                     solver->committedValue(decisionVars.at(i)));
    solver->endMove();

    solver->beginCommit();
    solver->query(violation);
    solver->endCommit();

    ++commits;
  }

  st.counters["commits_per_second"] = ::benchmark::Counter(
      static_cast<double>(commits), ::benchmark::Counter::kIsRate);
}

BENCHMARK_REGISTER_F(LinearAllDifferent, probe_single_swap)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(treeArguments);

/*
BENCHMARK_REGISTER_F(LinearAllDifferent, probe_all_swap)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(treeArguments);
/*
BENCHMARK_REGISTER_F(LinearAllDifferent,
commit_single_swap)->Apply(treeArguments);
//*/
}  // namespace atlantis::benchmark
