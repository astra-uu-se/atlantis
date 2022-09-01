#include <benchmark/benchmark.h>

#include <cassert>
#include <iostream>
#include <random>
#include <stack>
#include <utility>
#include <vector>

#include "constraints/allDifferent.hpp"
#include "core/propagationEngine.hpp"
#include "invariants/absDiff.hpp"
#include "invariants/linear.hpp"
#include "misc/logging.hpp"

class LinearTree : public benchmark::Fixture {
 private:
  struct TreeNode {
    size_t level;
    VarId id;
  };

  void createTree() {
    std::stack<TreeNode> treeNodes;
    output = engine->makeIntVar(0, lb, ub);
    treeNodes.push({1, output});
    vars.push_back(output);

#ifndef NDEBUG
    size_t numNodes = 1;
#endif

    while (!treeNodes.empty()) {
      std::vector<VarId> linearInputs(linearArgumentCount);
      for (size_t i = 0; i < linearArgumentCount; ++i) {
        linearInputs[i] = engine->makeIntVar(0, lb, ub);
        vars.push_back(linearInputs[i]);
      }
      TreeNode cur = treeNodes.top();
#ifndef NDEBUG
      numNodes += linearArgumentCount;
#endif
      treeNodes.pop();
      if (cur.level < treeHeight - 1) {
        for (VarId var : linearInputs) {
          treeNodes.push({cur.level + 1, var});
        }
      } else {
        assert(cur.level == treeHeight - 1);
        for (VarId var : linearInputs) {
          decisionVars.push_back(var);
        }
      }
      engine->makeInvariant<Linear>(linearInputs, cur.id);
      linearInputs.clear();
    }
#ifndef NDEBUG
    if (linearArgumentCount == 2) {
      assert(numNodes == (size_t(1) << treeHeight) - 1);
    }
#endif
  }

 public:
  std::unique_ptr<PropagationEngine> engine;
  VarId output;

  std::vector<VarId> vars;
  std::vector<VarId> decisionVars;  // Shared input vars to trees.

  std::random_device rd;

  std::mt19937 genVarIndex;
  std::mt19937 genDecisionVarIndex;
  std::mt19937 genDecisionVarValue;

  std::uniform_int_distribution<size_t> decisionVarIndexDist;
  std::uniform_int_distribution<size_t> varIndexDist;
  std::uniform_int_distribution<Int> decisionVarValueDist;

  size_t linearArgumentCount;
  size_t treeHeight;
  Int lb;
  Int ub;

  void probe(benchmark::State& st, size_t numMoves);
  void probeRnd(benchmark::State& st, size_t numMoves);
  void commit(benchmark::State& st, size_t numMoves);
  void commitRnd(benchmark::State& st, size_t numMoves);

  void SetUp(const ::benchmark::State& state) {
    setLogLevel(debug);

    engine = std::make_unique<PropagationEngine>();

    linearArgumentCount = state.range(1);
    treeHeight = state.range(2);
    lb = -1000;
    ub = 1000;

    engine->open();
    engine->setPropagationMode(
        static_cast<PropagationEngine::PropagationMode>(state.range(0)));

    createTree();

    logDebug("Created a tree of height " << treeHeight
                                         << ", each non-leaf node having "
                                         << linearArgumentCount << " children");
    engine->close();

    genDecisionVarIndex = std::mt19937(rd());
    genDecisionVarValue = std::mt19937(rd());
    genVarIndex = std::mt19937(rd());
    decisionVarIndexDist =
        std::uniform_int_distribution<size_t>(0, decisionVars.size() - 1);
    varIndexDist = std::uniform_int_distribution<size_t>(0, vars.size() - 1);
    decisionVarValueDist = std::uniform_int_distribution<Int>(lb, ub);
  }

  void TearDown(const ::benchmark::State&) {
    vars.clear();
    decisionVars.clear();
  }
};

void LinearTree::probe(benchmark::State& st, size_t numMoves) {
  size_t probes = 0;
  for (auto _ : st) {
    for (size_t i = 0; i < numMoves; ++i) {
      engine->beginMove();
      engine->setValue(
          decisionVars.at(decisionVarIndexDist(genDecisionVarIndex)),
          decisionVarValueDist(genDecisionVarValue));
      engine->endMove();
    }

    engine->beginProbe();
    engine->query(output);
    engine->endProbe();
    ++probes;
  }

  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
}

void LinearTree::probeRnd(benchmark::State& st, size_t numMoves) {
  size_t probes = 0;
  for (auto _ : st) {
    for (size_t i = 0; i < numMoves; ++i) {
      engine->beginMove();
      engine->setValue(
          decisionVars.at(decisionVarIndexDist(genDecisionVarIndex)),
          decisionVarValueDist(genDecisionVarValue));
      engine->endMove();
    }

    // Random query variable
    engine->beginProbe();
    engine->query(vars.at(varIndexDist(genVarIndex)));
    engine->endProbe();
    ++probes;
  }

  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
}

void LinearTree::commit(benchmark::State& st, size_t numMoves) {
  Int commits = 0;
  for (auto _ : st) {
    for (size_t i = 0; i < numMoves; ++i) {
      engine->beginMove();
      engine->setValue(
          decisionVars.at(decisionVarIndexDist(genDecisionVarIndex)),
          decisionVarValueDist(genDecisionVarValue));
      engine->endMove();
    }

    // Commit last output var
    engine->beginCommit();
    engine->query(output);
    engine->endCommit();
    ++commits;
  }

  st.counters["commits_per_second"] =
      benchmark::Counter(commits, benchmark::Counter::kIsRate);
}

void LinearTree::commitRnd(benchmark::State& st, size_t numMoves) {
  Int commits = 0;
  for (auto _ : st) {
    for (size_t i = 0; i < numMoves; ++i) {
      engine->beginMove();
      engine->setValue(
          decisionVars.at(decisionVarIndexDist(genDecisionVarIndex)),
          decisionVarValueDist(genDecisionVarValue));
      engine->endMove();
    }

    engine->beginCommit();
    engine->query(vars.at(varIndexDist(genVarIndex)));
    engine->endCommit();
    ++commits;
  }

  st.counters["commits_per_second"] =
      benchmark::Counter(commits, benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(LinearTree, probe_single_move)
(benchmark::State& st) { probe(std::ref(st), 1); }

BENCHMARK_DEFINE_F(LinearTree, probe_single_move_query_rnd)
(benchmark::State& st) { probeRnd(std::ref(st), 1); }

BENCHMARK_DEFINE_F(LinearTree, probe_swap)(benchmark::State& st) {
  probe(std::ref(st), 2);
}

BENCHMARK_DEFINE_F(LinearTree, probe_swap_query_rnd)
(benchmark::State& st) { probeRnd(std::ref(st), 2); }

BENCHMARK_DEFINE_F(LinearTree, probe_all_move)(benchmark::State& st) {
  probe(std::ref(st), decisionVars.size());
}

BENCHMARK_DEFINE_F(LinearTree, commit_single_move)
(benchmark::State& st) { commit(std::ref(st), 1); }

BENCHMARK_DEFINE_F(LinearTree, commit_single_move_query_rnd)
(benchmark::State& st) { commitRnd(std::ref(st), 1); }

BENCHMARK_DEFINE_F(LinearTree, commit_swap)(benchmark::State& st) {
  commit(std::ref(st), 2);
}

BENCHMARK_DEFINE_F(LinearTree, commit_swap_query_rnd)
(benchmark::State& st) { commitRnd(std::ref(st), 2); }

BENCHMARK_DEFINE_F(LinearTree, commit_all_move)(benchmark::State& st) {
  commit(std::ref(st), decisionVars.size());
}

BENCHMARK_DEFINE_F(LinearTree, commit_all_move_query_rnd)
(benchmark::State& st) { commitRnd(std::ref(st), decisionVars.size()); }

BENCHMARK_DEFINE_F(LinearTree, probe_all_move_query_rnd)
(benchmark::State& st) { probeRnd(std::ref(st), decisionVars.size()); }

/*

static void arguments(benchmark::internal::Benchmark* benchmark) {
  for (int treeHeight = 18; treeHeight <= 18; treeHeight += 2) {
    for (Int mode = 0; mode <= 2; ++mode) {
      benchmark->Args({mode, 2, treeHeight});
    }
  }
}

// -----------------------------------------
// Probing
// -----------------------------------------

BENCHMARK_REGISTER_F(LinearTree, probe_single_move)->Apply(arguments);
BENCHMARK_REGISTER_F(LinearTree, probe_single_move_query_rnd)->Apply(arguments);
BENCHMARK_REGISTER_F(LinearTree, probe_swap)->Apply(arguments);
BENCHMARK_REGISTER_F(LinearTree, probe_swap_query_rnd)->Apply(arguments);
BENCHMARK_REGISTER_F(LinearTree, probe_all_move)->Apply(arguments);
BENCHMARK_REGISTER_F(LinearTree, probe_all_move_query_rnd)->Apply(arguments);

/*
// -----------------------------------------
// Commit
// -----------------------------------------

BENCHMARK_REGISTER_F(LinearTree, commit_single_move)->Apply(arguments);
BENCHMARK_REGISTER_F(LinearTree, commit_single_move_query_rnd)
    ->Apply(arguments);
BENCHMARK_REGISTER_F(LinearTree, commit_swap)->Apply(arguments);
BENCHMARK_REGISTER_F(LinearTree, commit_swap_query_rnd)->Apply(arguments);
BENCHMARK_REGISTER_F(LinearTree, commit_all_move)->Apply(arguments);
BENCHMARK_REGISTER_F(LinearTree, commit_all_move_query_rnd)->Apply(arguments);

//*/