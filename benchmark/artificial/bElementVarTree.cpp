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
#include "invariants/elementVar.hpp"

class ElementVarTree : public benchmark::Fixture {
 private:
  struct TreeNode {
    size_t level;
    VarId id;
  };

  void createTree() {
    std::stack<TreeNode> treeNodes;
    output = engine->makeIntVar(0, 0, elementSize - 1);
    vars.push_back(output);

    treeNodes.push({1, output});

    while (!treeNodes.empty()) {
      TreeNode cur = treeNodes.top();
      treeNodes.pop();

      VarId indexVar = engine->makeIntVar(
          cur.level < treeHeight - 1 ? 0 : valueDist(genValue), 0,
          elementSize - 1);

      if (cur.level < treeHeight - 1) {
        treeNodes.push({cur.level + 1, indexVar});
      } else {
        assert(cur.level == treeHeight - 1);
        decisionVars.push_back(indexVar);
        indexDecisionVars.push_back(indexVar);
      }

      std::vector<VarId> elementInputs(elementSize, NULL_ID);

      for (size_t i = 0; i < elementInputs.size(); ++i) {
        elementInputs[i] = engine->makeIntVar(i, 0, elementInputs.size());
        if (cur.level < treeHeight - 1) {
          treeNodes.push({cur.level + 1, elementInputs[i]});
        } else {
          assert(cur.level == treeHeight - 1);
          decisionVars.push_back(elementInputs[i]);
        }
      }

      engine->makeInvariant<ElementVar>(indexVar, elementInputs, cur.id);
      elementInputs.clear();
    }
  }

 public:
  std::unique_ptr<PropagationEngine> engine;
  VarId output;

  std::vector<VarId> vars;
  std::vector<VarId> decisionVars;
  std::vector<VarId> indexDecisionVars;

  std::random_device rd;

  std::mt19937 genVarIndex;
  std::mt19937 genDecisionVarIndex;
  std::mt19937 genIndexDecisionVarIndex;
  std::mt19937 genValue;

  std::uniform_int_distribution<size_t> decisionVarIndexDist;
  std::uniform_int_distribution<size_t> varIndexDist;
  std::uniform_int_distribution<size_t> indexDecisionVarIndexDist;
  std::uniform_int_distribution<Int> valueDist;

  size_t treeHeight;
  size_t elementSize;

  void probe(benchmark::State& st, size_t numMoves);
  void probeRnd(benchmark::State& st, size_t numMoves);
  void commit(benchmark::State& st, size_t numMoves);
  void commitRnd(benchmark::State& st, size_t numMoves);

  void SetUp(const ::benchmark::State& state) {
    engine = std::make_unique<PropagationEngine>();

    treeHeight = state.range(1);
    elementSize = state.range(2);  // number of element inputs

    genValue = std::mt19937(rd());
    valueDist = std::uniform_int_distribution<Int>(0, elementSize - 1);

    engine->open();
    engine->setPropagationMode(
        static_cast<PropagationEngine::PropagationMode>(state.range(0)));

    createTree();

    engine->close();

    genDecisionVarIndex = std::mt19937(rd());
    genVarIndex = std::mt19937(rd());
    decisionVarIndexDist =
        std::uniform_int_distribution<size_t>(0, decisionVars.size() - 1);
    indexDecisionVarIndexDist =
        std::uniform_int_distribution<size_t>(0, indexDecisionVars.size() - 1);
    varIndexDist = std::uniform_int_distribution<size_t>(0, vars.size() - 1);
  }

  void TearDown(const ::benchmark::State&) {
    vars.clear();
    decisionVars.clear();
    indexDecisionVars.clear();
  }
};

void ElementVarTree::probe(benchmark::State& st, size_t numMoves) {
  size_t probes = 0;
  for (auto _ : st) {
    for (size_t i = 0; i < numMoves; ++i) {
      engine->beginMove();
      engine->setValue(
          decisionVars.at(decisionVarIndexDist(genDecisionVarIndex)),
          valueDist(genValue));
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

void ElementVarTree::commit(benchmark::State& st, size_t numMoves) {
  size_t commits = 0;
  for (auto _ : st) {
    for (size_t i = 0; i < numMoves; ++i) {
      engine->beginMove();
      engine->setValue(indexDecisionVars.at(
                           indexDecisionVarIndexDist(genIndexDecisionVarIndex)),
                       valueDist(genValue));
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

BENCHMARK_DEFINE_F(ElementVarTree, probe_single)
(benchmark::State& st) { probe(std::ref(st), 1); }

BENCHMARK_DEFINE_F(ElementVarTree, probe_double)(benchmark::State& st) {
  probe(std::ref(st), 2);
}

BENCHMARK_DEFINE_F(ElementVarTree, probe_all)(benchmark::State& st) {
  probe(std::ref(st), indexDecisionVars.size());
}

BENCHMARK_DEFINE_F(ElementVarTree, commit_single)
(benchmark::State& st) { commit(std::ref(st), 1); }

BENCHMARK_DEFINE_F(ElementVarTree, commit_double)(benchmark::State& st) {
  commit(std::ref(st), 2);
}

BENCHMARK_DEFINE_F(ElementVarTree, commit_all)(benchmark::State& st) {
  commit(std::ref(st), indexDecisionVars.size());
}

/*

static void arguments(benchmark::internal::Benchmark* benchmark) {
  for (int treeHeight = 6; treeHeight <= 12; treeHeight += 2) {
    for (int elementSize = 2; elementSize * treeHeight <= 36;
         elementSize += 2) {
      for (Int mode = 0; mode <= 2; ++mode) {
        benchmark->Args({mode, treeHeight, elementSize});
      }
    }
  }
}

// -----------------------------------------
// Probing
// -----------------------------------------

BENCHMARK_REGISTER_F(ElementVarTree, probe_single)->Apply(arguments);
BENCHMARK_REGISTER_F(ElementVarTree, probe_double)->Apply(arguments);
BENCHMARK_REGISTER_F(ElementVarTree, probe_all)->Apply(arguments);

/*
// -----------------------------------------
// Commit
// -----------------------------------------

BENCHMARK_REGISTER_F(ElementVarTree, commit_single)->Apply(arguments);
BENCHMARK_REGISTER_F(ElementVarTree, commit_double)->Apply(arguments);
BENCHMARK_REGISTER_F(ElementVarTree, commit_all)->Apply(arguments);

//*/