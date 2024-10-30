#include <benchmark/benchmark.h>

#include <cassert>
#include <functional>
#include <iostream>
#include <random>
#include <stack>
#include <utility>
#include <vector>

#include "../benchmark.hpp"
#include "atlantis/propagation/invariants/elementVar.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/utils/pow.hpp"

namespace atlantis::benchmark {

class ElementLinearTree : public ::benchmark::Fixture {
 private:
  struct TreeNode {
    size_t level;
    propagation::VarViewId id{propagation::NULL_ID};
  };

  void createTree() {
    std::stack<TreeNode> treeNodes;
    propagation::VarViewId output = solver->makeIntVar(0, lb, ub);
    elementInputVars.emplace_back(output);

    treeNodes.push({1, output});

#ifndef NDEBUG
    size_t numNodes = 1;
#endif

    while (!treeNodes.empty()) {
      std::vector<propagation::VarViewId> linearInputs;
      linearInputs.reserve(argumentCount);
      for (size_t i = 0; i < argumentCount; ++i) {
        linearInputs.emplace_back(solver->makeIntVar(0, lb, ub));
      }
      TreeNode cur = treeNodes.top();
#ifndef NDEBUG
      numNodes += argumentCount;
#endif
      treeNodes.pop();
      if (cur.level < treeHeight - 1) {
        for (propagation::VarViewId var : linearInputs) {
          treeNodes.push({cur.level + 1, var});
        }
      } else {
        assert(cur.level == treeHeight - 1);
        for (propagation::VarViewId var : linearInputs) {
          decisionVars.emplace_back(var);
        }
      }
      solver->makeInvariant<propagation::Linear>(*solver, cur.id,
                                                 std::move(linearInputs));
    }
#ifndef NDEBUG
    if (argumentCount == 2) {
      assert(numNodes == (size_t(1) << treeHeight) - 1);
    }
#endif
  }

 public:
  std::shared_ptr<propagation::Solver> solver;
  std::vector<propagation::VarViewId>
      elementInputVars;  // Ouput of each tree is input to Element.
  std::vector<propagation::VarViewId>
      decisionVars;  // Shared input vars to trees.

  propagation::VarViewId elementIndexVar = propagation::NULL_ID;
  propagation::VarViewId elementOutputVar = propagation::NULL_ID;

  std::random_device rd;
  std::mt19937 gen;
  std::uniform_int_distribution<size_t> decisionVarIndexDist;
  std::uniform_int_distribution<Int> decisionVarValueDist;
  std::uniform_int_distribution<Int> elementVarValueDist;
  Int treeCount{3};      // number of trees and width of element
  size_t treeHeight{0};  // height of each tree
  size_t argumentCount;
  Int lb{0};
  Int ub{1000};

  void SetUp(const ::benchmark::State& state) override {
    solver = std::make_shared<propagation::Solver>();

    treeHeight = state.range(0);
    argumentCount = state.range(1);

    solver->open();
    setSolverMode(*solver, static_cast<int>(state.range(2)));

    elementIndexVar = solver->makeIntVar(0, 0, treeCount - 1);
    elementOutputVar = solver->makeIntVar(0, lb, ub);

    // Create the trees
    for (Int i = 0; i < treeCount; ++i) {
      createTree();
    }

    solver->makeInvariant<propagation::ElementVar>(
        *solver, elementOutputVar, elementIndexVar,
        std::vector<propagation::VarViewId>(elementInputVars));
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

// probe_single_non_index_var
// probe_single_index_var
// probe_single_move_index_input

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

/*

static void arguments(::benchmark::internal::Benchmark* benchmark) {
  for (int treeCount = 2; treeCount <= 10; treeCount += 2) {
    for (int treeHeight = 2;
         treeHeight <= 10 && pow(treeHeight, 2) <= 2048; treeHeight += 2) {
      for (Int mode = 0; mode <= 3; ++mode) {
        benchmark->Args({treeCount, treeHeight, mode});
      }
#ifndef NDEBUG
      return;
#endif
    }
  }
}
*/

BENCHMARK_REGISTER_F(ElementLinearTree, probe_single_non_index_var)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultTreeArguments);

BENCHMARK_REGISTER_F(ElementLinearTree, probe_single_index_var)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultTreeArguments);

//*/
}  // namespace atlantis::benchmark
