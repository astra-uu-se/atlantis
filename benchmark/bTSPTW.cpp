#include <benchmark/benchmark.h>

#include <constraints/lessEqual.hpp>
#include <core/propagationEngine.hpp>
#include <invariants/elementConst.hpp>
#include <invariants/elementVar.hpp>
#include <invariants/linear.hpp>
#include <iostream>
#include <random>
#include <utility>
#include <vector>
#include <views/intOffsetView.hpp>

class TSPTW : public benchmark::Fixture {
 public:
  std::unique_ptr<PropagationEngine> engine;
  std::vector<VarId> pred;
  std::vector<VarId> timeToPrev;
  std::vector<VarId> arrivalPrev;
  std::vector<VarId> arrivalTime;
  std::vector<std::vector<Int>> dist;
  VarId totalDist;

  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<> distribution;
  int n;
  const int MAX_TIME = 100000;

  std::vector<VarId> violation;
  VarId totalViolation;

  void SetUp(const ::benchmark::State& state) {
    engine = std::make_unique<PropagationEngine>();
    n = state.range(1);

    logDebug(n);
    engine->open();

    switch (state.range(0)) {
      case 0:
        engine->setPropagationMode(
            PropagationEngine::PropagationMode::INPUT_TO_OUTPUT);
        break;
      case 1:
        engine->setPropagationMode(PropagationEngine::PropagationMode::MIXED);
        break;
      case 2:
        engine->setPropagationMode(
            PropagationEngine::PropagationMode::OUTPUT_TO_INPUT);
        break;
    }

    for (int i = 0; i < n; ++i) {
      dist.push_back(std::vector<Int>());
      for (int j = 0; j < n; ++j) {
        dist[i].push_back(i * j);
      }
    }

    for (int i = 0; i < n; ++i) {
      pred.emplace_back(engine->makeIntVar((i + 1) % n, 0, n - 1));
      timeToPrev.emplace_back(engine->makeIntVar(0, 0, MAX_TIME));
      arrivalTime.emplace_back(engine->makeIntVar(0, 0, MAX_TIME));
      arrivalPrev.emplace_back(engine->makeIntVar(0, 0, MAX_TIME));
      violation.emplace_back(engine->makeIntVar(0, 0, MAX_TIME));
    }

    // Ignore index 0
    for (int i = 1; i < n; ++i) {
      // timeToPrev[i] = dist[i][pred[i]]
      engine->makeInvariant<ElementConst>(pred[i], dist[i], timeToPrev[i]);
      // arrivalPrev[i] = arrivalTime[pred[i]]
    }

    // Ignore index 0
    for (int i = 1; i < n; ++i) {
      // arrivalPrev[i] = arrivalTime[pred[i]]
      engine->makeInvariant<ElementVar>(pred[i], arrivalTime, arrivalPrev[i]);
      // arrivalTime[i] = arrivalPrev[i] + timeToPrev[i]
      engine->makeInvariant<Linear>(
          std::vector<VarId>({arrivalPrev[i], timeToPrev[i]}), arrivalTime[i]);
    }

    // totalDist = sum(timeToPrev)
    totalDist = engine->makeIntVar(0, 0, MAX_TIME);
    engine->makeInvariant<Linear>(timeToPrev, totalDist);

    VarId leqConst = engine->makeIntVar(100, 100, 100);
    for (int i = 0; i < n; ++i) {
      engine->makeConstraint<LessEqual>(violation[i], arrivalTime[i], leqConst);
    }

    totalViolation = engine->makeIntVar(0, 0, MAX_TIME * n);
    engine->makeInvariant<Linear>(violation, totalViolation);

    engine->close();

    gen = std::mt19937(rd());

    distribution = std::uniform_int_distribution<>{0, n - 1};
  }

  void TearDown(const ::benchmark::State&) {
    pred.clear();
    timeToPrev.clear();
    arrivalPrev.clear();
    arrivalTime.clear();
    dist.clear();
    violation.clear();
  }
};

BENCHMARK_DEFINE_F(TSPTW, probe_all_relocate)(benchmark::State& st) {
  Int probes = 0;
  for (auto _ : st) {
    for (int i = 0; i < n; ++i) {
      for (int j = 0; j < n; ++j) {
        if (i == j || engine->getCommittedValue(pred[i]) == j) {
          continue;
        }
        engine->beginMove();
        engine->setValue(pred[i],
                         engine->getCommittedValue(
                             pred[engine->getCommittedValue(pred[i])]));
        engine->setValue(pred[j], engine->getCommittedValue(pred[i]));
        engine->setValue(pred[engine->getCommittedValue(pred[i])],
                         engine->getCommittedValue(pred[j]));
        engine->endMove();

        engine->beginQuery();
        engine->query(totalDist);
        engine->query(totalViolation);
        engine->endQuery();
        ++probes;
      }
    }
  }
  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
}

///*
static void arguments(benchmark::internal::Benchmark* b) {
  for (int i = 10; i <= 100; i += 10) {
    for (int mode = 0; mode <= 2; ++mode) {
      b->Args({mode, i});
    }
  }
}

BENCHMARK_REGISTER_F(TSPTW, probe_all_relocate)
    ->Apply(arguments)
    ->Unit(benchmark::kMillisecond);
//*/