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
#include "constraints/allDifferent.hpp"
#include "core/propagationEngine.hpp"
#include "invariants/absDiff.hpp"
#include "invariants/linear.hpp"

class FoldableBinaryTree : public benchmark::Fixture {
 private:
  VarId randomVariable() { return vars.at(std::rand() % vars.size()); }
  VarId createTree() {
    VarId output = engine->makeIntVar(0, lb, ub);
    vars.push_back(output);

    VarId prev = output;

    for (size_t level = 0; level < treeHeight; ++level) {
      VarId left = engine->makeIntVar(0, lb, ub);
      VarId right = engine->makeIntVar(0, lb, ub);
      vars.push_back(left);
      vars.push_back(right);

      engine->makeInvariant<Linear>(*engine, prev,
                                    std::vector<VarId>{left, right});
      if (level == treeHeight - 1) {
        decisionVars.push_back(left);
      }
      decisionVars.push_back(right);
      prev = left;
    }
    assert(decisionVars.size() == treeHeight + 1);
    return output;
  }

 public:
  std::unique_ptr<PropagationEngine> engine;
  std::vector<VarId> vars;
  std::vector<VarId> decisionVars;
  VarId queryVar;
  std::random_device rd;

  std::mt19937 genValue;

  std::uniform_int_distribution<Int> decisionVarValueDist;

  size_t treeHeight;
  Int lb;
  Int ub;

  void probe(benchmark::State& st, size_t moveCount);
  void probeRnd(benchmark::State& st, size_t moveCount);
  void commit(benchmark::State& st, size_t moveCount);
  void commitRnd(benchmark::State& st, size_t moveCount);

  void SetUp(const ::benchmark::State& state) {
    std::srand(std::time(0));

    engine = std::make_unique<PropagationEngine>();

    treeHeight = state.range(0);  // Tree height
    lb = -1000;
    ub = 1000;

    engine->open();
    setEngineModes(*engine, state.range(1));

    queryVar = createTree();

    engine->close();

    genValue = std::mt19937(rd());
    decisionVarValueDist = std::uniform_int_distribution<Int>(lb, ub);
  }

  void TearDown(const ::benchmark::State&) {
    decisionVars.clear();
    vars.clear();
  }
};

void FoldableBinaryTree::probe(benchmark::State& st, size_t moveCount) {
  size_t probes = 0;
  moveCount = std::min(moveCount, decisionVars.size());
  for (auto _ : st) {
    st.PauseTiming();
    std::random_shuffle(decisionVars.begin(), decisionVars.end());
    st.ResumeTiming();

    engine->beginMove();
    for (size_t i = 0; i < moveCount; ++i) {
      engine->setValue(decisionVars.at(i), decisionVarValueDist(genValue));
    }
    engine->endMove();

    // Query queryVar var
    engine->beginProbe();
    engine->query(queryVar);
    engine->endProbe();
    ++probes;
  }

  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
}

void FoldableBinaryTree::probeRnd(benchmark::State& st, size_t moveCount) {
  size_t probes = 0;
  moveCount = std::min(moveCount, decisionVars.size());
  for (auto _ : st) {
    st.PauseTiming();
    std::random_shuffle(decisionVars.begin(), decisionVars.end());
    st.ResumeTiming();

    engine->beginMove();
    for (size_t i = 0; i < moveCount; ++i) {
      if (i >= decisionVars.size()) {
        logWarning("i: " << i);
      }
      engine->setValue(decisionVars.at(i), decisionVarValueDist(genValue));
    }
    engine->endMove();

    // Random query variable
    engine->beginProbe();
    engine->query(randomVariable());
    engine->endProbe();
    ++probes;
  }

  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
}

void FoldableBinaryTree::commit(benchmark::State& st, size_t moveCount) {
  size_t commits = 0;
  moveCount = std::min(moveCount, decisionVars.size());
  for (auto _ : st) {
    st.PauseTiming();
    std::random_shuffle(decisionVars.begin(), decisionVars.end());

    engine->beginMove();
    for (size_t i = 0; i < moveCount; ++i) {
      engine->setValue(decisionVars.at(i), decisionVarValueDist(genValue));
    }
    engine->endMove();

    st.ResumeTiming();

    // Commit last queryVar var
    engine->beginCommit();
    engine->query(queryVar);
    engine->endCommit();
    ++commits;
  }

  st.counters["seconds_per_commit"] = benchmark::Counter(
      commits, benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
}

void FoldableBinaryTree::commitRnd(benchmark::State& st, size_t moveCount) {
  size_t commits = 0;
  moveCount = std::min(moveCount, decisionVars.size());
  for (auto _ : st) {
    st.PauseTiming();
    std::random_shuffle(decisionVars.begin(), decisionVars.end());

    engine->beginMove();
    for (size_t i = 0; i < moveCount; ++i) {
      engine->setValue(decisionVars.at(i), decisionVarValueDist(genValue));
    }
    engine->endMove();

    st.ResumeTiming();

    // Query random variable
    engine->beginCommit();
    engine->query(randomVariable());
    engine->endCommit();
    ++commits;
  }

  st.counters["seconds_per_commit"] = benchmark::Counter(
      commits, benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
}

BENCHMARK_DEFINE_F(FoldableBinaryTree, probe_single)
(benchmark::State& st) { probe(std::ref(st), 1); }

BENCHMARK_DEFINE_F(FoldableBinaryTree, probe_single_query_rnd)
(benchmark::State& st) { probeRnd(std::ref(st), 1); }

BENCHMARK_DEFINE_F(FoldableBinaryTree, probe_double)
(benchmark::State& st) { probe(std::ref(st), 2); }

BENCHMARK_DEFINE_F(FoldableBinaryTree, probe_double_query_rnd)
(benchmark::State& st) { probeRnd(std::ref(st), 2); }

BENCHMARK_DEFINE_F(FoldableBinaryTree, probe_all)
(benchmark::State& st) { probe(std::ref(st), st.range(1) + 1); }

BENCHMARK_DEFINE_F(FoldableBinaryTree, probe_all_query_rnd)
(benchmark::State& st) { probeRnd(std::ref(st), st.range(1) + 1); }

BENCHMARK_DEFINE_F(FoldableBinaryTree, commit_move_single)
(benchmark::State& st) { commit(std::ref(st), 1); }

BENCHMARK_DEFINE_F(FoldableBinaryTree, commit_move_single_query_rnd)
(benchmark::State& st) { commitRnd(std::ref(st), 1); }

BENCHMARK_DEFINE_F(FoldableBinaryTree, commit_move_two)
(benchmark::State& st) { commit(std::ref(st), 2); }

BENCHMARK_DEFINE_F(FoldableBinaryTree, commit_move_two_query_rnd)
(benchmark::State& st) { commitRnd(std::ref(st), 2); }

BENCHMARK_DEFINE_F(FoldableBinaryTree, commit_move_all)
(benchmark::State& st) { commit(std::ref(st), decisionVars.size()); }

BENCHMARK_DEFINE_F(FoldableBinaryTree, commit_move_all_query_rnd)
(benchmark::State& st) { commitRnd(std::ref(st), decisionVars.size()); }

/*

// -----------------------------------------
// Probing
// ------------------------------------------

static void arguments(benchmark::internal::Benchmark* benchmark) {
  for (int treeHeight = 2; treeHeight <= 10; treeHeight += 2) {
    for (Int mode = 0; mode <= 3; ++mode) {
      benchmark->Args({treeHeight, mode});
    }
#ifndef NDEBUG
    return;
#endif
  }
}

BENCHMARK_REGISTER_F(FoldableBinaryTree, probe_single)
    ->Unit(benchmark::kMillisecond)
    ->Apply(arguments);
BENCHMARK_REGISTER_F(FoldableBinaryTree, probe_single_query_rnd)
    ->Unit(benchmark::kMillisecond)
    ->Apply(arguments);

/*
BENCHMARK_REGISTER_F(FoldableBinaryTree, probe_double)
    ->Unit(benchmark::kMillisecond)
    ->Apply(arguments);
BENCHMARK_REGISTER_F(FoldableBinaryTree, probe_double_query_rnd)
    ->Unit(benchmark::kMillisecond)
    ->Apply(arguments);
BENCHMARK_REGISTER_F(FoldableBinaryTree, probe_all)
    ->Unit(benchmark::kMillisecond)
    ->Apply(arguments);
BENCHMARK_REGISTER_F(FoldableBinaryTree, probe_all_query_rnd)
    ->Unit(benchmark::kMillisecond)
    ->Apply(arguments);

/*
// -----------------------------------------
// Commit
// ------------------------------------------

// BENCHMARK_REGISTER_F(FoldableBinaryTree, commit_move_single)
//    ->ArgsProduct({benchmark::CreateRange(4, 4, 2),
//                   benchmark::CreateRange(0, 0, 0)});
BENCHMARK_REGISTER_F(FoldableBinaryTree, commit_move_single_query_rnd)
    ->DenseRange(4, 4, 2);
BENCHMARK_REGISTER_F(FoldableBinaryTree, commit_move_two)->DenseRange(4, 4, 2);
BENCHMARK_REGISTER_F(FoldableBinaryTree, commit_move_two_query_rnd)
    ->DenseRange(4, 4, 2);
BENCHMARK_REGISTER_F(FoldableBinaryTree, commit_move_all)->DenseRange(4, 4, 2);
BENCHMARK_REGISTER_F(FoldableBinaryTree, commit_move_all_query_rnd)
    ->DenseRange(4, 4, 2);

//*/