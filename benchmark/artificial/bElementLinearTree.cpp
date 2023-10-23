#include <benchmark/benchmark.h>

#include <cassert>
#include <functional>
#include <iostream>
#include <random>
#include <stack>
#include <utility>
#include <vector>

#include "../benchmark.hpp"
#include "misc/logging.hpp"
#include "propagation/invariants/elementVar.hpp"
#include "propagation/invariants/linear.hpp"
#include "propagation/propagationEngine.hpp"

namespace atlantis::benchmark {

class ElementLinearTree : public ::benchmark::Fixture {
 private:
  struct TreeNode {
    size_t level;
    propagation::VarId id;
  };

  void createTree() {
    std::stack<TreeNode> treeNodes;
    propagation::VarId output = engine->makeIntVar(0, lb, ub);
    elementInputVars.push_back(output);

    treeNodes.push({1, output});

#ifndef NDEBUG
    size_t numNodes = 1;
#endif

    while (!treeNodes.empty()) {
      std::vector<propagation::VarId> linearInputs(linearArgumentCount);
      for (size_t i = 0; i < linearArgumentCount; ++i) {
        linearInputs[i] = engine->makeIntVar(0, lb, ub);
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
      engine->makeInvariant<propagation::Linear>(*engine, cur.id, linearInputs);
      linearInputs.clear();
    }
#ifndef NDEBUG
    if (linearArgumentCount == 2) {
      assert(numNodes == (size_t(1) << treeHeight) - 1);
    }
#endif
  }

 public:
  std::unique_ptr<propagation::PropagationEngine> engine;
  std::vector<propagation::VarId>
      elementInputVars;  // Ouput of each tree is input to Element.
  std::vector<propagation::VarId> decisionVars;  // Shared input vars to trees.

  propagation::VarId elementIndexVar = propagation::NULL_ID;
  propagation::VarId elementOutputVar = propagation::NULL_ID;

  std::random_device rd;
  std::mt19937 gen;
  std::uniform_int_distribution<size_t> decisionVarIndexDist;
  std::uniform_int_distribution<Int> decisionVarValueDist;
  std::uniform_int_distribution<Int> elementVarValueDist;
  Int treeCount;      // number of trees and width of element
  size_t treeHeight;  // height of each tree
  size_t linearArgumentCount;
  int lb, ub;

  void SetUp(const ::benchmark::State& state) {
    engine = std::make_unique<propagation::PropagationEngine>();

    lb = 0;
    ub = 1000;

    linearArgumentCount = 2;  // state.range(0);
    treeCount = state.range(0);
    treeHeight = state.range(1);

    engine->open();
    setEngineModes(*engine, state.range(2));

    elementIndexVar = engine->makeIntVar(0, 0, treeCount - 1);
    elementOutputVar = engine->makeIntVar(0, lb, ub);

    // Create the trees
    for (Int i = 0; i < treeCount; ++i) {
      createTree();
    }
    logDebug("Created " << treeCount << " trees of height " << treeHeight
                        << ", each non-leaf node having " << linearArgumentCount
                        << " children");

    engine->makeInvariant<propagation::ElementVar>(
        *engine, elementOutputVar, elementIndexVar, elementInputVars);
    engine->close();
    gen = std::mt19937(rd());
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

// probe_single_non_index_var
// probe_single_index_var
// probe_single_move_index_input

BENCHMARK_DEFINE_F(ElementLinearTree, probe_single_non_index_var)
(::benchmark::State& st) {
  size_t probes = 0;
  for (auto _ : st) {
    // Perform move
    engine->beginMove();
    engine->setValue(decisionVars.at(decisionVarIndexDist(gen)),
                     decisionVarValueDist(gen));
    engine->endMove();

    engine->beginProbe();
    engine->query(elementOutputVar);
    engine->endProbe();

    ++probes;
  }
  st.counters["probes_per_second"] =
      ::benchmark::Counter(probes, ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(ElementLinearTree, probe_single_index_var)
(::benchmark::State& st) {
  size_t probes = 0;
  for (auto _ : st) {
    engine->beginMove();
    engine->setValue(elementIndexVar, elementVarValueDist(gen));
    engine->endMove();

    engine->beginProbe();
    engine->query(elementOutputVar);
    engine->endProbe();

    ++probes;
  }
  st.counters["probes_per_second"] =
      ::benchmark::Counter(probes, ::benchmark::Counter::kIsRate);
}

/*

static void arguments(::benchmark::internal::Benchmark* benchmark) {
  for (int treeCount = 2; treeCount <= 10; treeCount += 2) {
    for (int treeHeight = 2;
         treeHeight <= 10 && std::pow(treeHeight, 2) <= 2048; treeHeight += 2) {
      for (Int mode = 0; mode <= 3; ++mode) {
        benchmark->Args({treeCount, treeHeight, mode});
      }
#ifndef NDEBUG
      return;
#endif
    }
  }
}

BENCHMARK_REGISTER_F(ElementLinearTree, probe_single_non_index_var)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(arguments);

BENCHMARK_REGISTER_F(ElementLinearTree, probe_single_index_var)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(arguments);

//*/
}  // namespace atlantis::benchmark