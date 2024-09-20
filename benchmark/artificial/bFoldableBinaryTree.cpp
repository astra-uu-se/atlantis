#include <benchmark/benchmark.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>
#include <stack>
#include <utility>
#include <vector>

#include "../benchmark.hpp"
#include "atlantis/propagation/invariants/absDiff.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"

namespace atlantis::benchmark {

class FoldableBinaryTree : public ::benchmark::Fixture {
 private:
  propagation::VarViewId randomVar() {
    return vars.at(std::rand() % vars.size());
  }
  propagation::VarViewId createTree() {
    propagation::VarViewId output = solver->makeIntVar(0, lb, ub);
    vars.emplace_back(output);

    propagation::VarViewId prev = output;

    for (size_t level = 0; level < treeHeight; ++level) {
      propagation::VarViewId left = solver->makeIntVar(0, lb, ub);
      propagation::VarViewId right = solver->makeIntVar(0, lb, ub);
      vars.emplace_back(left);
      vars.emplace_back(right);

      solver->makeInvariant<propagation::Linear>(
          *solver, prev, std::vector<propagation::VarViewId>{left, right});
      if (level == treeHeight - 1) {
        decisionVars.emplace_back(left);
      }
      decisionVars.emplace_back(right);
      prev = left;
    }
    assert(decisionVars.size() == treeHeight + 1);
    return output;
  }

 public:
  std::shared_ptr<propagation::Solver> solver;
  std::vector<propagation::VarViewId> vars;
  std::vector<propagation::VarViewId> decisionVars;
  VarViewId queryVar{propagation::NULL_ID};
  std::random_device rd;

  std::mt19937 genValue;

  std::uniform_int_distribution<Int> decisionVarValueDist;

  size_t treeHeight{0};
  Int lb{-2};
  Int ub{2};

  void probe(::benchmark::State& st, size_t moveCount);
  void probeRnd(::benchmark::State& st, size_t moveCount);
  void commit(::benchmark::State& st, size_t moveCount);
  void commitRnd(::benchmark::State& st, size_t moveCount);

  void SetUp(const ::benchmark::State& state) override {
    solver = std::make_shared<propagation::Solver>();

    treeHeight = state.range(0);  // Tree height

    solver->open();
    setSolverMode(*solver, static_cast<int>(state.range(1)));

    queryVar = createTree();

    solver->close();

    genValue = std::mt19937(rd());
    decisionVarValueDist = std::uniform_int_distribution<Int>(lb, ub);
  }

  void TearDown(const ::benchmark::State&) override {
    decisionVars.clear();
    vars.clear();
  }
};

void FoldableBinaryTree::probe(::benchmark::State& st, size_t moveCount) {
  size_t probes = 0;
  moveCount = std::min(moveCount, decisionVars.size());
  for ([[maybe_unused]] const auto& _ : st) {
    st.PauseTiming();
    std::shuffle(decisionVars.begin(), decisionVars.end(), genValue);
    st.ResumeTiming();

    solver->beginMove();
    for (size_t i = 0; i < moveCount; ++i) {
      solver->setValue(decisionVars.at(i), decisionVarValueDist(genValue));
    }
    solver->endMove();

    // Query queryVar var
    solver->beginProbe();
    solver->query(queryVar);
    solver->endProbe();
    ++probes;
  }

  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

void FoldableBinaryTree::probeRnd(::benchmark::State& st, size_t moveCount) {
  size_t probes = 0;
  moveCount = std::min(moveCount, decisionVars.size());
  for ([[maybe_unused]] const auto& _ : st) {
    st.PauseTiming();
    std::shuffle(decisionVars.begin(), decisionVars.end(), genValue);
    st.ResumeTiming();

    solver->beginMove();
    for (size_t i = 0; i < moveCount; ++i) {
      if (i >= decisionVars.size()) {
        logWarning("i: " << i);
      }
      solver->setValue(decisionVars.at(i), decisionVarValueDist(genValue));
    }
    solver->endMove();

    // Random query variable
    solver->beginProbe();
    solver->query(randomVar());
    solver->endProbe();
    ++probes;
  }

  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

void FoldableBinaryTree::commit(::benchmark::State& st, size_t moveCount) {
  size_t commits = 0;
  moveCount = std::min(moveCount, decisionVars.size());
  for ([[maybe_unused]] const auto& _ : st) {
    st.PauseTiming();
    std::shuffle(decisionVars.begin(), decisionVars.end(), genValue);

    solver->beginMove();
    for (size_t i = 0; i < moveCount; ++i) {
      solver->setValue(decisionVars.at(i), decisionVarValueDist(genValue));
    }
    solver->endMove();

    st.ResumeTiming();

    // Commit last queryVar var
    solver->beginCommit();
    solver->query(queryVar);
    solver->endCommit();
    ++commits;
  }

  st.counters["seconds_per_commit"] = ::benchmark::Counter(
      static_cast<double>(commits),
      ::benchmark::Counter::kIsRate | ::benchmark::Counter::kInvert);
}

void FoldableBinaryTree::commitRnd(::benchmark::State& st, size_t moveCount) {
  size_t commits = 0;
  moveCount = std::min(moveCount, decisionVars.size());
  for ([[maybe_unused]] const auto& _ : st) {
    st.PauseTiming();
    std::shuffle(decisionVars.begin(), decisionVars.end(), genValue);

    solver->beginMove();
    for (size_t i = 0; i < moveCount; ++i) {
      solver->setValue(decisionVars.at(i), decisionVarValueDist(genValue));
    }
    solver->endMove();

    st.ResumeTiming();

    // Query random variable
    solver->beginCommit();
    solver->query(randomVar());
    solver->endCommit();
    ++commits;
  }

  st.counters["seconds_per_commit"] = ::benchmark::Counter(
      static_cast<double>(commits),
      ::benchmark::Counter::kIsRate | ::benchmark::Counter::kInvert);
}

BENCHMARK_DEFINE_F(FoldableBinaryTree, probe_single)
(::benchmark::State& st) { probe(std::ref(st), 1); }

BENCHMARK_DEFINE_F(FoldableBinaryTree, probe_single_query_rnd)
(::benchmark::State& st) { probeRnd(std::ref(st), 1); }

BENCHMARK_DEFINE_F(FoldableBinaryTree, probe_double)
(::benchmark::State& st) { probe(std::ref(st), 2); }

BENCHMARK_DEFINE_F(FoldableBinaryTree, probe_double_query_rnd)
(::benchmark::State& st) { probeRnd(std::ref(st), 2); }

BENCHMARK_DEFINE_F(FoldableBinaryTree, probe_all)
(::benchmark::State& st) { probe(std::ref(st), st.range(1) + 1); }

BENCHMARK_DEFINE_F(FoldableBinaryTree, probe_all_query_rnd)
(::benchmark::State& st) { probeRnd(std::ref(st), st.range(1) + 1); }

BENCHMARK_DEFINE_F(FoldableBinaryTree, commit_move_single)
(::benchmark::State& st) { commit(std::ref(st), 1); }

BENCHMARK_DEFINE_F(FoldableBinaryTree, commit_move_single_query_rnd)
(::benchmark::State& st) { commitRnd(std::ref(st), 1); }

BENCHMARK_DEFINE_F(FoldableBinaryTree, commit_move_two)
(::benchmark::State& st) { commit(std::ref(st), 2); }

BENCHMARK_DEFINE_F(FoldableBinaryTree, commit_move_two_query_rnd)
(::benchmark::State& st) { commitRnd(std::ref(st), 2); }

BENCHMARK_DEFINE_F(FoldableBinaryTree, commit_move_all)
(::benchmark::State& st) { commit(std::ref(st), decisionVars.size()); }

BENCHMARK_DEFINE_F(FoldableBinaryTree, commit_move_all_query_rnd)
(::benchmark::State& st) { commitRnd(std::ref(st), decisionVars.size()); }

/*

// -----------------------------------------
// Probing
// ------------------------------------------

BENCHMARK_REGISTER_F(FoldableBinaryTree, probe_single)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultTreeArguments);
BENCHMARK_REGISTER_F(FoldableBinaryTree, probe_single_query_rnd)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultTreeArguments);

/*
BENCHMARK_REGISTER_F(FoldableBinaryTree, probe_double)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultTreeArguments);
BENCHMARK_REGISTER_F(FoldableBinaryTree, probe_double_query_rnd)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultTreeArguments);
BENCHMARK_REGISTER_F(FoldableBinaryTree, probe_all)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultTreeArguments);
BENCHMARK_REGISTER_F(FoldableBinaryTree, probe_all_query_rnd)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultTreeArguments);

/*
// -----------------------------------------
// Commit
// ------------------------------------------

// BENCHMARK_REGISTER_F(FoldableBinaryTree, commit_move_single)
//    ->ArgsProduct({::benchmark::CreateRange(4, 4, 2),
//                   ::benchmark::CreateRange(0, 0, 0)});
BENCHMARK_REGISTER_F(FoldableBinaryTree, commit_move_single_query_rnd)
    ->DenseRange(4, 4, 2);
BENCHMARK_REGISTER_F(FoldableBinaryTree, commit_move_two)->DenseRange(4, 4, 2);
BENCHMARK_REGISTER_F(FoldableBinaryTree, commit_move_two_query_rnd)
    ->DenseRange(4, 4, 2);
BENCHMARK_REGISTER_F(FoldableBinaryTree, commit_move_all)->DenseRange(4, 4, 2);
BENCHMARK_REGISTER_F(FoldableBinaryTree, commit_move_all_query_rnd)
    ->DenseRange(4, 4, 2);

//*/
}  // namespace atlantis::benchmark
