#include <benchmark/benchmark.h>

#include <cassert>
#include <iostream>
#include <random>
#include <stack>
#include <utility>
#include <vector>

#include "../benchmark.hpp"
#include "atlantis/propagation/invariants/absDiff.hpp"
#include "atlantis/propagation/invariants/elementVar.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"
#include "atlantis/utils/pow.hpp"

namespace atlantis::benchmark {

class ElementVarTree : public ::benchmark::Fixture {
 private:
  struct TreeNode {
    size_t level;
    Int indexVal{-1};
    propagation::VarViewId id{propagation::NULL_ID};
  };

  void createTree() {
    dynamicSearchVars.reserve(dynamicInputCount);

    for (size_t i = 0; i < dynamicInputCount; ++i) {
      dynamicSearchVars.emplace_back(
          solver->makeIntVar(dynamicValDist(gen), lb, ub));
      vars.emplace_back(dynamicSearchVars.back());
      searchVars.emplace_back(dynamicSearchVars.back());
    }

    std::stack<TreeNode> treeNodes;
    output = solver->makeIntVar(0, 0, static_cast<Int>(dynamicInputCount) - 1);
    vars.emplace_back(output);

    treeNodes.push({1, -1, output});

    while (!treeNodes.empty()) {
      TreeNode cur = treeNodes.top();
      treeNodes.pop();

      propagation::VarViewId indexVar = solver->makeIntVar(
          cur.indexVal < 0 ? staticValDist(gen) : cur.indexVal, 0,
          static_cast<Int>(dynamicInputCount) - 1);

      searchVars.emplace_back(indexVar);
      staticSearchVars.emplace_back(indexVar);

      std::vector<propagation::VarViewId> elementInputs;
      elementInputs.reserve(dynamicInputCount);

      for (size_t i = 0; i < dynamicInputCount; ++i) {
        if (cur.level < treeHeight - 1) {
          elementInputs.emplace_back(solver->makeIntVar(
              static_cast<Int>(i), 0, static_cast<Int>(dynamicInputCount) - 1));
          treeNodes.push(
              {cur.level + 1,
               cur.indexVal >= 0 ? cur.indexVal : static_cast<Int>(i),
               elementInputs[i]});
        } else {
          assert(cur.level == treeHeight - 1);
          elementInputs.emplace_back(dynamicSearchVars[i]);
        }
      }

      solver->makeInvariant<propagation::ElementVar>(
          *solver, cur.id, indexVar, std::move(elementInputs), 0);
    }
  }

 public:
  std::shared_ptr<propagation::Solver> solver;
  propagation::VarViewId output{propagation::NULL_ID};

  std::vector<propagation::VarViewId> vars;
  std::vector<propagation::VarViewId> searchVars;
  std::vector<propagation::VarViewId> staticSearchVars;
  std::vector<propagation::VarViewId> dynamicSearchVars;

  std::random_device rd;

  std::mt19937 gen;

  std::uniform_int_distribution<size_t> dynamicVarDist;
  std::uniform_int_distribution<size_t> staticVarDist;

  std::uniform_int_distribution<Int> staticValDist;
  std::uniform_int_distribution<Int> dynamicValDist;

  size_t treeHeight{0};
  size_t dynamicInputCount{0};

  Int lb{-1000};
  Int ub{1000};

  void probe(::benchmark::State& st, size_t numMoves);
  void probeStatic(::benchmark::State& st, size_t numMoves);
  void commit(::benchmark::State& st, size_t numMoves);

  void SetUp(const ::benchmark::State& state) override {
    solver = std::make_shared<propagation::Solver>();

    treeHeight = state.range(0);
    dynamicInputCount = state.range(1);  // number of element inputs

    gen = std::mt19937(rd());
    staticValDist = std::uniform_int_distribution<Int>(
        0, static_cast<Int>(dynamicInputCount) - 1);
    dynamicValDist = std::uniform_int_distribution<Int>(lb, ub);

    solver->open();
    setSolverMode(*solver, static_cast<int>(state.range(2)));

    createTree();

    solver->close();

    dynamicVarDist =
        std::uniform_int_distribution<size_t>(0, dynamicSearchVars.size() - 1);
    staticVarDist =
        std::uniform_int_distribution<size_t>(0, staticSearchVars.size() - 1);
  }

  void TearDown(const ::benchmark::State&) override {
    vars.clear();
    searchVars.clear();
    staticSearchVars.clear();
    dynamicSearchVars.clear();
  }
};

void ElementVarTree::probe(::benchmark::State& st, size_t numMoves) {
  size_t probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    for (size_t i = 0; i < numMoves; ++i) {
      solver->beginMove();
      solver->setValue(dynamicSearchVars[dynamicVarDist(gen)],
                       dynamicValDist(gen));
      solver->endMove();
    }

    solver->beginProbe();
    solver->query(output);
    solver->endProbe();
    ++probes;
  }

  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

void ElementVarTree::probeStatic(::benchmark::State& st, size_t numMoves) {
  size_t probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    for (size_t i = 0; i < numMoves; ++i) {
      solver->beginMove();
      solver->setValue(staticSearchVars[staticVarDist(gen)],
                       staticValDist(gen));
      solver->endMove();
    }

    solver->beginProbe();
    solver->query(output);
    solver->endProbe();
    ++probes;
  }

  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

void ElementVarTree::commit(::benchmark::State& st, size_t numMoves) {
  size_t commits = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    for (size_t i = 0; i < numMoves; ++i) {
      solver->beginMove();
      solver->setValue(staticSearchVars.at(staticVarDist(gen)),
                       staticValDist(gen));
      solver->endMove();
    }

    // Commit last output var
    solver->beginCommit();
    solver->query(output);
    solver->endCommit();
    ++commits;
  }

  st.counters["commits_per_second"] = ::benchmark::Counter(
      static_cast<double>(commits), ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(ElementVarTree, probe_single)
(::benchmark::State& st) { probe(std::ref(st), 1); }

BENCHMARK_DEFINE_F(ElementVarTree, probe_double)(::benchmark::State& st) {
  probe(std::ref(st), 2);
}

BENCHMARK_DEFINE_F(ElementVarTree, probe_all)(::benchmark::State& st) {
  probe(std::ref(st), staticSearchVars.size());
}

BENCHMARK_DEFINE_F(ElementVarTree, probe_all_static)(::benchmark::State& st) {
  probeStatic(std::ref(st), staticSearchVars.size());
}

BENCHMARK_DEFINE_F(ElementVarTree, commit_single)
(::benchmark::State& st) { commit(std::ref(st), 1); }

BENCHMARK_DEFINE_F(ElementVarTree, commit_double)(::benchmark::State& st) {
  commit(std::ref(st), 2);
}

BENCHMARK_DEFINE_F(ElementVarTree, commit_all)(::benchmark::State& st) {
  commit(std::ref(st), staticSearchVars.size());
}

/*

static void arguments(::benchmark::internal::Benchmark* benchmark) {
  for (int treeHeight = 2; treeHeight <= 10; treeHeight += 2) {
    for (int dynamicInputCount = 2;
         dynamicInputCount <= 10 && pow(treeHeight, dynamicInputCount) <=
2048;
         ++dynamicInputCount) {
      for (Int mode = 0; mode <= 3; ++mode) {
        benchmark->Args({treeHeight, dynamicInputCount, mode});
      }
#ifndef NDEBUG
      return;
#endif
    }
  }
}
*/
// -----------------------------------------
// Probing
// -----------------------------------------
BENCHMARK_REGISTER_F(ElementVarTree, probe_single)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultTreeArguments);

/*
BENCHMARK_REGISTER_F(ElementVarTree, probe_all_static)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultTreeArguments);

/*
BENCHMARK_REGISTER_F(ElementVarTree, probe_all)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultTreeArguments);


BENCHMARK_REGISTER_F(ElementVarTree, probe_double)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultTreeArguments);

//*/

/*
// -----------------------------------------
// Commit
// -----------------------------------------

static void commitArguments(::benchmark::internal::Benchmark* benchmark) {
  for (int treeHeight = 6; treeHeight <= 6; treeHeight += 2) {
    for (int dynamicInputCount = 2; dynamicInputCount * treeHeight <= 12;
         dynamicInputCount += 2) {
      benchmark->Args({treeHeight, dynamicInputCount. 0});
    }
  }
}

BENCHMARK_REGISTER_F(ElementVarTree,
commit_single)->Unit(::benchmark::kMillisecond)->Apply(commitArguments);
BENCHMARK_REGISTER_F(ElementVarTree,
commit_double)->Unit(::benchmark::kMillisecond)->Apply(commitArguments);
BENCHMARK_REGISTER_F(ElementVarTree,
commit_all)->Unit(::benchmark::kMillisecond)->Apply(commitArguments);

//*/
}  // namespace atlantis::benchmark
