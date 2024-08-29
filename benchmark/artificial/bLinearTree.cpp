#include <benchmark/benchmark.h>

#include <cassert>
#include <iostream>
#include <random>
#include <stack>
#include <utility>
#include <vector>

#include "../benchmark.hpp"
#include "atlantis/misc/logging.hpp"
#include "atlantis/propagation/invariants/absDiff.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"

namespace atlantis::benchmark {

class LinearTree : public ::benchmark::Fixture {
 private:
  struct TreeNode {
    size_t level;
    propagation::VarId id;
  };

  void createTree() {
    std::stack<TreeNode> treeNodes;
    output = solver->makeIntVar(0, lb, ub);
    treeNodes.push({1, output});
    vars.push_back(output);

#ifndef NDEBUG
    size_t numNodes = 1;
#endif

    while (!treeNodes.empty()) {
      std::vector<propagation::VarId> linearInputs(linearArgumentCount);
      for (size_t i = 0; i < linearArgumentCount; ++i) {
        linearInputs[i] = solver->makeIntVar(0, lb, ub);
        vars.push_back(linearInputs[i]);
      }
      TreeNode cur = treeNodes.top();
#ifndef NDEBUG
      numNodes += linearArgumentCount;
#endif
      treeNodes.pop();
      if (cur.level < treeHeight - 1) {
        for (propagation::VarId var : linearInputs) {
          treeNodes.push({cur.level + 1, var});
        }
      } else {
        assert(cur.level == treeHeight - 1);
        for (propagation::VarId var : linearInputs) {
          decisionVars.push_back(var);
        }
      }
      solver->makeInvariant<propagation::Linear>(*solver, cur.id,
                                                 std::move(linearInputs));
    }
#ifndef NDEBUG
    if (linearArgumentCount == 2) {
      assert(numNodes == (size_t(1) << treeHeight) - 1);
    }
#endif
  }

 public:
  std::unique_ptr<propagation::Solver> solver;
  propagation::VarId output;

  std::vector<propagation::VarId> vars;
  std::vector<propagation::VarId> decisionVars;  // Shared input vars to trees.

  std::random_device rd;

  std::mt19937 gen;

  std::uniform_int_distribution<size_t> decisionVarIndexDist;
  std::uniform_int_distribution<size_t> varIndexDist;
  std::uniform_int_distribution<Int> decisionVarValueDist;

  size_t linearArgumentCount{0};
  size_t treeHeight{0};
  Int lb{0};
  Int ub{0};

  void probe(::benchmark::State& st, size_t moveSize);
  void probe_query_rnd(::benchmark::State& st, size_t moveSize);
  void commit(::benchmark::State& st, size_t moveSize);
  void commit_query_rnd(::benchmark::State& st, size_t moveSize);

  void SetUp(const ::benchmark::State& state) override {
    solver = std::make_unique<propagation::Solver>();

    linearArgumentCount = state.range(0);
    treeHeight = state.range(1);
    lb = -1000;
    ub = 1000;

    solver->open();
    setSolverMode(*solver, static_cast<int>(state.range(2)));

    createTree();

    logDebug("Created a tree of height " << treeHeight
                                         << ", each non-leaf node having "
                                         << linearArgumentCount << " children");
    solver->close();

    gen = std::mt19937(rd());
    decisionVarIndexDist =
        std::uniform_int_distribution<size_t>(0, decisionVars.size() - 1);
    varIndexDist = std::uniform_int_distribution<size_t>(0, vars.size() - 1);
    decisionVarValueDist = std::uniform_int_distribution<Int>(lb, ub);
  }

  void TearDown(const ::benchmark::State&) override {
    vars.clear();
    decisionVars.clear();
  }
};

void LinearTree::probe(::benchmark::State& st, size_t moveSize) {
  size_t probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    st.PauseTiming();
    std::shuffle(decisionVars.begin(), decisionVars.end(), gen);

    for (size_t i = 0; i < moveSize; ++i) {
      solver->setValue(decisionVars[i], decisionVarValueDist(gen));
    }
    solver->endMove();

    st.ResumeTiming();

    solver->beginProbe();
    solver->query(output);
    solver->endProbe();
    ++probes;
  }

  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

void LinearTree::probe_query_rnd(::benchmark::State& st, size_t moveSize) {
  size_t probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    st.PauseTiming();

    std::shuffle(decisionVars.begin(), decisionVars.end(), gen);

    solver->beginMove();
    for (size_t i = 0; i < moveSize; ++i) {
      solver->setValue(decisionVars[i], decisionVarValueDist(gen));
    }
    solver->endMove();

    st.ResumeTiming();

    // Random query variable
    solver->beginProbe();
    solver->query(vars.at(varIndexDist(gen)));
    solver->endProbe();
    ++probes;
  }

  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

void LinearTree::commit(::benchmark::State& st, size_t moveSize) {
  Int commits = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    st.PauseTiming();
    std::shuffle(decisionVars.begin(), decisionVars.end(), gen);

    solver->beginMove();
    for (size_t i = 0; i < moveSize; ++i) {
      solver->setValue(decisionVars.at(decisionVarIndexDist(gen)),
                       decisionVarValueDist(gen));
    }
    solver->endMove();
    st.ResumeTiming();

    // Commit last output var
    solver->beginCommit();
    solver->query(output);
    solver->endCommit();
    ++commits;
  }

  st.counters["commits_per_second"] = ::benchmark::Counter(
      static_cast<double>(commits), ::benchmark::Counter::kIsRate);
}

void LinearTree::commit_query_rnd(::benchmark::State& st, size_t moveSize) {
  Int commits = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    st.PauseTiming();
    std::shuffle(decisionVars.begin(), decisionVars.end(), gen);
    solver->beginMove();
    for (size_t i = 0; i < moveSize; ++i) {
      solver->setValue(decisionVars.at(decisionVarIndexDist(gen)),
                       decisionVarValueDist(gen));
    }
    solver->endMove();
    st.ResumeTiming();

    solver->beginCommit();
    solver->query(vars.at(varIndexDist(gen)));
    solver->endCommit();
    ++commits;
  }

  st.counters["commits_per_second"] = ::benchmark::Counter(
      static_cast<double>(commits), ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(LinearTree, probe_single)
(::benchmark::State& st) { probe(std::ref(st), 1); }

BENCHMARK_DEFINE_F(LinearTree, probe_single_query_rnd)
(::benchmark::State& st) { probe_query_rnd(std::ref(st), 1); }

BENCHMARK_DEFINE_F(LinearTree, probe_swap)(::benchmark::State& st) {
  probe(std::ref(st), 2);
}

BENCHMARK_DEFINE_F(LinearTree, probe_swap_query_rnd)
(::benchmark::State& st) { probe_query_rnd(std::ref(st), 2); }

BENCHMARK_DEFINE_F(LinearTree, probe_all_move)(::benchmark::State& st) {
  probe(std::ref(st), decisionVars.size());
}

BENCHMARK_DEFINE_F(LinearTree, commit_single)
(::benchmark::State& st) { commit(std::ref(st), 1); }

BENCHMARK_DEFINE_F(LinearTree, commit_single_query_rnd)
(::benchmark::State& st) { commit_query_rnd(std::ref(st), 1); }

BENCHMARK_DEFINE_F(LinearTree, commit_swap)(::benchmark::State& st) {
  commit(std::ref(st), 2);
}

BENCHMARK_DEFINE_F(LinearTree, commit_swap_query_rnd)
(::benchmark::State& st) { commit_query_rnd(std::ref(st), 2); }

BENCHMARK_DEFINE_F(LinearTree, commit_all_move)(::benchmark::State& st) {
  commit(std::ref(st), decisionVars.size());
}

BENCHMARK_DEFINE_F(LinearTree, commit_all_query_rnd)
(::benchmark::State& st) {
  commit_query_rnd(std::ref(st), decisionVars.size());
}

BENCHMARK_DEFINE_F(LinearTree, probe_all_query_rnd)
(::benchmark::State& st) { probe_query_rnd(std::ref(st), decisionVars.size()); }

// -----------------------------------------
// Probing
// -----------------------------------------

BENCHMARK_REGISTER_F(LinearTree, probe_single)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(treeArguments);
/*
BENCHMARK_REGISTER_F(LinearTree, probe_single_query_rnd)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(treeArguments);

//*/
/*
BENCHMARK_REGISTER_F(LinearTree, probe_swap)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(treeArguments);
BENCHMARK_REGISTER_F(LinearTree, probe_swap_query_rnd)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(treeArguments);
BENCHMARK_REGISTER_F(LinearTree, probe_all_move)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(treeArguments);
BENCHMARK_REGISTER_F(LinearTree, probe_all_query_rnd)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(treeArguments);

/*
// -----------------------------------------
// Commit
// -----------------------------------------

BENCHMARK_REGISTER_F(LinearTree, commit_single)->Apply(treeArguments);
BENCHMARK_REGISTER_F(LinearTree, commit_single_query_rnd)
    ->Apply(treeArguments);
BENCHMARK_REGISTER_F(LinearTree, commit_swap)->Apply(treeArguments);
BENCHMARK_REGISTER_F(LinearTree, commit_swap_query_rnd)->Apply(treeArguments);
BENCHMARK_REGISTER_F(LinearTree, commit_all_move)->Apply(treeArguments);
BENCHMARK_REGISTER_F(LinearTree, commit_all_query_rnd)->Apply(treeArguments);

//*/
}  // namespace atlantis::benchmark
