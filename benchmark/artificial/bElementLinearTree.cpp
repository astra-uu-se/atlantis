#include <benchmark/benchmark.h>

#include <cassert>
#include <functional>
#include <iostream>
#include <random>
#include <stack>
#include <utility>
#include <vector>

#include "../benchmark.hpp"
#include "atlantis/misc/logging.hpp"
#include "atlantis/propagation/invariants/elementVar.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::benchmark {

class ElementLinearTree : public ::benchmark::Fixture {
 private:
  struct TreeNode {
    size_t level;
    propagation::VarId id;
  };

  void createTree() {
    std::stack<TreeNode> treeNodes;
    propagation::VarId output = solver->makeIntVar(0, lb, ub);
    elementInputVars.push_back(output);

    treeNodes.push({1, output});

#ifndef NDEBUG
    size_t numNodes = 1;
#endif

    while (!treeNodes.empty()) {
      std::vector<propagation::VarId> linearInputs(linearArgumentCount);
      for (size_t i = 0; i < linearArgumentCount; ++i) {
        linearInputs[i] = solver->makeIntVar(0, lb, ub);
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
  Int treeCount{0};      // number of trees and width of element
  size_t treeHeight{0};  // height of each tree
  size_t linearArgumentCount{0};
  int lb{0};
  int ub{0};

  void SetUp(const ::benchmark::State& state) override {
    solver = std::make_unique<propagation::Solver>();

    lb = 0;
    ub = 1000;

    linearArgumentCount = 2;  // state.range(0);
    treeCount = state.range(0);
    treeHeight = state.range(1);

    solver->open();
    setSolverMode(*solver, static_cast<int>(state.range(2)));

    elementIndexVar = solver->makeIntVar(0, 0, treeCount - 1);
    elementOutputVar = solver->makeIntVar(0, lb, ub);

    // Create the trees
    for (Int i = 0; i < treeCount; ++i) {
      createTree();
    }
    logDebug("Created " << treeCount << " trees of height " << treeHeight
                        << ", each non-leaf node having " << linearArgumentCount
                        << " children");

    solver->makeInvariant<propagation::ElementVar>(
        *solver, elementOutputVar, elementIndexVar,
        std::vector<propagation::VarId>(elementInputVars));
    solver->close();
    gen = std::mt19937(rd());
    decisionVarIndexDist =
        std::uniform_int_distribution<size_t>{0, decisionVars.size() - 1};
    decisionVarValueDist = std::uniform_int_distribution<Int>{lb, ub};
    elementVarValueDist = std::uniform_int_distribution<Int>{0, treeCount - 1};
  }

  void TearDown(const ::benchmark::State&) override {
    elementInputVars.clear();
    decisionVars.clear();
  }
};

BENCHMARK_DEFINE_F(ElementLinearTree, probe_single_non_index_var)
(::benchmark::State& st) {
  size_t probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    // Perform move
    solver->beginMove();
    solver->setValue(decisionVars.at(decisionVarIndexDist(gen)),
                     decisionVarValueDist(gen));
    solver->endMove();

    solver->beginProbe();
    solver->query(elementOutputVar);
    solver->endProbe();

    ++probes;
  }
  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(ElementLinearTree, probe_single_index_var)
(::benchmark::State& st) {
  size_t probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    solver->beginMove();
    solver->setValue(elementIndexVar, elementVarValueDist(gen));
    solver->endMove();

    solver->beginProbe();
    solver->query(elementOutputVar);
    solver->endProbe();

    ++probes;
  }
  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

BENCHMARK_REGISTER_F(ElementLinearTree, probe_single_non_index_var)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(treeArguments);

BENCHMARK_REGISTER_F(ElementLinearTree, probe_single_index_var)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(treeArguments);

/*
//*/
}  // namespace atlantis::benchmark
