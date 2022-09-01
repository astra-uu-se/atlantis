#include <benchmark/benchmark.h>

#include <cassert>
#include <functional>
#include <iostream>
#include <random>
#include <stack>
#include <utility>
#include <vector>

#include "core/propagationEngine.hpp"
#include "invariants/elementVar.hpp"
#include "invariants/linear.hpp"
#include "misc/logging.hpp"

class ElementLinearTree : public benchmark::Fixture {
 private:
  struct TreeNode {
    size_t level;
    VarId id;
  };

  void createTree() {
    std::stack<TreeNode> treeNodes;
    VarId output = engine->makeIntVar(0, lb, ub);
    elementInputVars.push_back(output);

    treeNodes.push({1, output});

#ifndef NDEBUG
    size_t numNodes = 1;
#endif

    while (!treeNodes.empty()) {
      std::vector<VarId> linearInputs(linearArgumentCount);
      for (size_t i = 0; i < linearArgumentCount; ++i) {
        linearInputs[i] = engine->makeIntVar(0, lb, ub);
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
  std::vector<VarId>
      elementInputVars;             // Ouput of each tree is input to Element.
  std::vector<VarId> decisionVars;  // Shared input vars to trees.

  VarId elementIndexVar = NULL_ID;
  VarId elementOutputVar = NULL_ID;

  std::random_device rd;
  std::mt19937 genDecisionVarIndex;
  std::mt19937 genDecisionVarValue;
  std::mt19937 genElementVarValue;
  std::uniform_int_distribution<size_t> decisionVarIndexDist;
  std::uniform_int_distribution<Int> decisionVarValueDist;
  std::uniform_int_distribution<Int> elementVarValueDist;
  Int treeCount;      // number of trees and width of element
  size_t treeHeight;  // height of each tree
  size_t linearArgumentCount;
  int lb, ub;

  void SetUp(const ::benchmark::State& state) {
    // setLogLevel(debug);

    engine = std::make_unique<PropagationEngine>();

    lb = 0;
    ub = 1000;

    engine->open();
    engine->setPropagationMode(
        static_cast<PropagationEngine::PropagationMode>(state.range(0)));

    linearArgumentCount = state.range(1);
    treeCount = state.range(2);
    treeHeight = state.range(3);

    elementIndexVar = engine->makeIntVar(0, 0, treeCount - 1);
    elementOutputVar = engine->makeIntVar(0, lb, ub);

    // Create the trees
    for (Int i = 0; i < treeCount; ++i) {
      createTree();
    }
    logDebug("Created " << treeCount << " trees of height " << treeHeight
                        << ", each non-leaf node having " << linearArgumentCount
                        << " children");

    engine->makeInvariant<ElementVar>(elementIndexVar, elementInputVars,
                                      elementOutputVar);
    engine->close();
    genDecisionVarIndex = std::mt19937(rd());
    genDecisionVarValue = std::mt19937(rd());
    genElementVarValue = std::mt19937(rd());
    decisionVarIndexDist =
        std::uniform_int_distribution<size_t>{0, decisionVars.size() - 1};
    decisionVarValueDist = std::uniform_int_distribution<Int>{lb, ub};
    elementVarValueDist = std::uniform_int_distribution<Int>{0, treeCount - 1};
  }

  void TearDown(const ::benchmark::State&) {
    elementInputVars.clear();
    decisionVars.clear();
  }
};

// probe_single_move_decision_var
// probing_single_move_index_var
// probing_single_move_index_input

BENCHMARK_DEFINE_F(ElementLinearTree, probe_single_move_decision_var)
(benchmark::State& st) {
  size_t probes = 0;
  for (auto _ : st) {
    // Perform move
    engine->beginMove();
    engine->setValue(decisionVars.at(decisionVarIndexDist(genDecisionVarIndex)),
                     decisionVarValueDist(genDecisionVarValue));
    engine->endMove();

    engine->beginProbe();
    engine->query(elementOutputVar);
    engine->endProbe();

    ++probes;
  }
  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(ElementLinearTree, probing_single_move_index_var)
(benchmark::State& st) {
  size_t probes = 0;
  for (auto _ : st) {
    engine->beginMove();
    engine->setValue(elementIndexVar, elementVarValueDist(genElementVarValue));
    engine->endMove();

    engine->beginProbe();
    engine->query(elementOutputVar);
    engine->endProbe();

    ++probes;
  }
  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
}

///*

static void arguments(benchmark::internal::Benchmark* benchmark) {
  for (int treeCount = 2; treeCount <= 10; treeCount += 2) {
    for (int treeHeight = 6; treeHeight <= 12; treeHeight += 2) {
      for (Int mode = 0; mode <= 2; ++mode) {
        benchmark->Args({mode, 2, treeCount, treeHeight});
      }
    }
  }
}

BENCHMARK_REGISTER_F(ElementLinearTree, probe_single_move_decision_var)
    ->Apply(arguments);

BENCHMARK_REGISTER_F(ElementLinearTree, probing_single_move_index_var)
    ->Apply(arguments);

//*/