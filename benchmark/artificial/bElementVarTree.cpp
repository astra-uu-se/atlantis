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

namespace atlantis::benchmark {

class ElementVarTree : public ::benchmark::Fixture {
 private:
  struct TreeNode {
    size_t level;
    propagation::VarId id;
  };

  void createTree() {
    std::stack<TreeNode> treeNodes;
    output = solver->makeIntVar(0, 0, static_cast<Int>(elementSize) - 1);
    vars.push_back(output);

    treeNodes.push({1, output});

    while (!treeNodes.empty()) {
      TreeNode cur = treeNodes.top();
      treeNodes.pop();

      propagation::VarId indexVar =
          solver->makeIntVar(cur.level < treeHeight - 1 ? 0 : valueDist(gen), 0,
                             static_cast<Int>(elementSize) - 1);

      if (cur.level < treeHeight - 1) {
        treeNodes.push({cur.level + 1, indexVar});
      } else {
        assert(cur.level == treeHeight - 1);
        decisionVars.push_back(indexVar);
        indexDecisionVars.push_back(indexVar);
      }

      std::vector<propagation::VarId> elementInputs(elementSize,
                                                    propagation::NULL_ID);

      for (size_t i = 0; i < elementInputs.size(); ++i) {
        elementInputs[i] = solver->makeIntVar(
            static_cast<Int>(i), 0, static_cast<Int>(elementInputs.size()));
        if (cur.level < treeHeight - 1) {
          treeNodes.push({cur.level + 1, elementInputs[i]});
        } else {
          assert(cur.level == treeHeight - 1);
          decisionVars.push_back(elementInputs[i]);
        }
      }

      solver->makeInvariant<propagation::ElementVar>(*solver, cur.id, indexVar,
                                                     std::move(elementInputs));
    }
  }

 public:
  std::unique_ptr<propagation::Solver> solver;
  propagation::VarId output;

  std::vector<propagation::VarId> vars;
  std::vector<propagation::VarId> decisionVars;
  std::vector<propagation::VarId> indexDecisionVars;

  std::random_device rd;

  std::mt19937 gen;

  std::uniform_int_distribution<Int> valueDist;

  size_t treeHeight{0};
  size_t elementSize{0};

  void probe_index(::benchmark::State& st, size_t moveSize);
  void probe_any(::benchmark::State& st, size_t moveSize);

  void commit_index(::benchmark::State& st, size_t moveSize);
  void commit_any(::benchmark::State& st, size_t moveSize);

  void SetUp(const ::benchmark::State& state) override {
    solver = std::make_unique<propagation::Solver>();

    treeHeight = state.range(0);
    elementSize = state.range(1);  // number of element inputs

    gen = std::mt19937(rd());
    valueDist = std::uniform_int_distribution<Int>(
        0, static_cast<Int>(elementSize) - 1);

    solver->open();
    setSolverMode(*solver, static_cast<int>(state.range(2)));

    createTree();

    solver->close();
  }

  void TearDown(const ::benchmark::State&) override {
    vars.clear();
    decisionVars.clear();
    indexDecisionVars.clear();
  }
};

void ElementVarTree::probe_index(::benchmark::State& st, size_t moveSize) {
  size_t probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    st.PauseTiming();

    std::shuffle(decisionVars.begin(), decisionVars.end(), gen);

    solver->beginMove();
    for (size_t i = 0; i < moveSize; ++i) {
      solver->setValue(decisionVars.at(i), valueDist(gen));
    }
    solver->endMove();

    solver->beginProbe();
    solver->query(output);
    solver->endProbe();
    ++probes;
  }

  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

void ElementVarTree::probe_any(::benchmark::State& st, size_t moveSize) {
  size_t probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    st.PauseTiming();

    std::shuffle(decisionVars.begin(), decisionVars.end(), gen);

    solver->beginMove();
    for (size_t i = 0; i < moveSize; ++i) {
      solver->setValue(decisionVars.at(i), valueDist(gen));
    }
    solver->endMove();

    solver->beginProbe();
    solver->query(output);
    solver->endProbe();
    ++probes;
  }

  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

void ElementVarTree::commit_index(::benchmark::State& st, size_t moveSize) {
  size_t commits = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    st.PauseTiming();
    std::shuffle(indexDecisionVars.begin(), indexDecisionVars.end(), gen);

    solver->beginMove();
    for (size_t i = 0; i < moveSize; ++i) {
      solver->setValue(indexDecisionVars.at(i), valueDist(gen));
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

void ElementVarTree::commit_any(::benchmark::State& st, size_t moveSize) {
  size_t commits = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    st.PauseTiming();
    std::shuffle(decisionVars.begin(), decisionVars.end(), gen);

    solver->beginMove();
    for (size_t i = 0; i < moveSize; ++i) {
      solver->setValue(decisionVars.at(i), valueDist(gen));
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

BENCHMARK_DEFINE_F(ElementVarTree, probe_index_single)
(::benchmark::State& st) { probe_index(std::ref(st), 1); }

BENCHMARK_DEFINE_F(ElementVarTree, probe_index_double)(::benchmark::State& st) {
  probe_index(std::ref(st), 2);
}

BENCHMARK_DEFINE_F(ElementVarTree, probe_index_all)(::benchmark::State& st) {
  probe_index(std::ref(st), indexDecisionVars.size());
}

BENCHMARK_DEFINE_F(ElementVarTree, probe_any_single)
(::benchmark::State& st) { probe_any(std::ref(st), 1); }

BENCHMARK_DEFINE_F(ElementVarTree, probe_any_double)(::benchmark::State& st) {
  probe_any(std::ref(st), 2);
}

BENCHMARK_DEFINE_F(ElementVarTree, probe_any_all)(::benchmark::State& st) {
  probe_any(std::ref(st), indexDecisionVars.size());
}

BENCHMARK_DEFINE_F(ElementVarTree, commit_index_single)
(::benchmark::State& st) { commit_index(std::ref(st), 1); }

BENCHMARK_DEFINE_F(ElementVarTree, commit_index_double)
(::benchmark::State& st) { commit_index(std::ref(st), 2); }

BENCHMARK_DEFINE_F(ElementVarTree, commit_index_all)(::benchmark::State& st) {
  commit_index(std::ref(st), indexDecisionVars.size());
}

BENCHMARK_DEFINE_F(ElementVarTree, commit_any_single)
(::benchmark::State& st) { commit_any(std::ref(st), 1); }

BENCHMARK_DEFINE_F(ElementVarTree, commit_any_double)
(::benchmark::State& st) { commit_any(std::ref(st), 2); }

BENCHMARK_DEFINE_F(ElementVarTree, commit_any_all)(::benchmark::State& st) {
  commit_any(std::ref(st), indexDecisionVars.size());
}

// -----------------------------------------
// Probing
// -----------------------------------------

BENCHMARK_REGISTER_F(ElementVarTree, probe_index_single)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(treeArguments);
/*
BENCHMARK_REGISTER_F(ElementVarTree, probe_index_double)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(treeArguments);
BENCHMARK_REGISTER_F(ElementVarTree, probe_index_all)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(treeArguments);

//*/

/*
// -----------------------------------------
// Commit
// -----------------------------------------

static void commitArguments(::benchmark::internal::Benchmark* benchmark) {
  for (int treeHeight = 6; treeHeight <= 6; treeHeight += 2) {
    for (int elementSize = 2; elementSize * treeHeight <= 12;
         elementSize += 2) {
      benchmark->Args({treeHeight, elementSize. 0});
    }
  }
}

BENCHMARK_REGISTER_F(ElementVarTree,
commit_index_single)->Unit(::benchmark::kMillisecond)->Apply(treeArguments);
BENCHMARK_REGISTER_F(ElementVarTree,
commit_index_double)->Unit(::benchmark::kMillisecond)->Apply(treeArguments);
BENCHMARK_REGISTER_F(ElementVarTree,
commit_index_all)->Unit(::benchmark::kMillisecond)->Apply(treeArguments);

//*/
}  // namespace atlantis::benchmark
