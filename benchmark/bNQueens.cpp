#include <benchmark/benchmark.h>

#include <constraints/allDifferent.hpp>
#include <core/propagationEngine.hpp>
#include <invariants/linear.hpp>
#include <iostream>
#include <random>
#include <utility>
#include <vector>
#include <views/intOffsetView.hpp>

class Queens : public benchmark::Fixture {
 public:
  std::unique_ptr<PropagationEngine> engine;
  std::vector<VarId> queens;
  std::vector<VarId> q_offset_plus;
  std::vector<VarId> q_offset_minus;
  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<> distribution;
  int n;

  VarId violation1 = NULL_ID;
  VarId violation2 = NULL_ID;
  VarId violation3 = NULL_ID;
  VarId total_violation = NULL_ID;

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
      const VarId q = engine->makeIntVar(i, 0, n - 1);
      queens.push_back(q);
      q_offset_minus.push_back(
          engine->makeIntView<IntOffsetView>(q, -i)->getId());
      q_offset_plus.push_back(
          engine->makeIntView<IntOffsetView>(q, i)->getId());
    }

    violation1 = engine->makeIntVar(0, 0, n);
    violation2 = engine->makeIntVar(0, 0, n);
    violation3 = engine->makeIntVar(0, 0, n);

    engine->makeConstraint<AllDifferent>(violation1, queens);
    engine->makeConstraint<AllDifferent>(violation2, q_offset_minus);
    engine->makeConstraint<AllDifferent>(violation3, q_offset_plus);

    total_violation = engine->makeIntVar(0, 0, 3 * n);

    engine->makeInvariant<Linear>(
        std::vector<Int>{1, 1, 1},
        std::vector<VarId>{violation1, violation2, violation3},
        total_violation);

    engine->close();

    gen = std::mt19937(rd());

    distribution = std::uniform_int_distribution<>{0, n - 1};
  }

  void TearDown(const ::benchmark::State&) {
    queens.clear();
    q_offset_minus.clear();
    q_offset_plus.clear();
  }

  std::string instanceToString() {
    std::string str = "Queens: ";
    for (auto q : queens) {
      str += std::to_string(engine->getCommittedValue(q)) + ", ";
    }
    return str;
  }
};

BENCHMARK_DEFINE_F(Queens, probing_single_swap)(benchmark::State& st) {
  for (auto _ : st) {
    const int i = distribution(gen);
    const int j = distribution(gen);
    const Int oldI = engine->getCommittedValue(queens.at(i));
    const Int oldJ = engine->getCommittedValue(queens.at(j));
    // Perform random swap
    engine->beginMove();
    engine->setValue(queens.at(i), oldJ);
    engine->setValue(queens.at(j), oldI);
    engine->endMove();

    engine->beginQuery();
    engine->query(total_violation);
    engine->endQuery();
  }
}

BENCHMARK_DEFINE_F(Queens, probing_all_swap)(benchmark::State& st) {
  int probes = 0;
  for (auto _ : st) {
    for (size_t i = 0; i < static_cast<size_t>(n); ++i) {
      for (size_t j = i + 1; j < static_cast<size_t>(n); ++j) {
        const Int oldI = engine->getCommittedValue(queens.at(i));
        const Int oldJ = engine->getCommittedValue(queens.at(j));
        engine->beginMove();
        engine->setValue(queens.at(i), oldJ);
        engine->setValue(queens.at(j), oldI);
        engine->endMove();

        engine->beginQuery();
        engine->query(total_violation);
        engine->endQuery();

        ++probes;
      }
    }
  }
  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(Queens, solve)(benchmark::State& st) {
  Int it = 0;
  Int probes = 0;

  std::vector<Int> tabu;
  tabu.assign(n, 0);
  const Int tenure = 10;
  bool done = false;

  for (auto _ : st) {
    while (it < 100000 && !done) {
      size_t bestI = 0;
      size_t bestJ = 0;
      Int bestViol = n;
      for (size_t i = 0; i < static_cast<size_t>(n); ++i) {
        for (size_t j = i + 1; j < static_cast<size_t>(n); ++j) {
          if (tabu[i] > it && tabu[j] > it) {
            continue;
          }
          const Int oldI = engine->getCommittedValue(queens.at(i));
          const Int oldJ = engine->getCommittedValue(queens.at(j));
          engine->beginMove();
          engine->setValue(queens.at(i), oldJ);
          engine->setValue(queens.at(j), oldI);
          engine->endMove();

          engine->beginQuery();
          engine->query(total_violation);
          engine->endQuery();

          ++probes;

          Int newValue = engine->getNewValue(total_violation);
          if (newValue <= bestViol) {
            bestViol = newValue;
            bestI = i;
            bestJ = j;
          }
        }
      }

      const Int oldI = engine->getCommittedValue(queens.at(bestI));
      const Int oldJ = engine->getCommittedValue(queens.at(bestJ));
      engine->beginMove();
      engine->setValue(queens.at(bestI), oldJ);
      engine->setValue(queens.at(bestJ), oldI);
      engine->endMove();

      engine->beginCommit();
      engine->query(total_violation);
      engine->endCommit();

      tabu[bestI] = it + tenure;
      tabu[bestJ] = it + tenure;
      if (bestViol == 0) {
        done = true;
      }
      ++it;
    }
  }

  st.counters["it"] = benchmark::Counter(it);

  st.counters["it_per_s"] = benchmark::Counter(it, benchmark::Counter::kIsRate);
  st.counters["probes_per_s"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
  st.counters["solved"] = benchmark::Counter(done);
  logDebug(instanceToString());
}

/*
static void arguments(benchmark::internal::Benchmark* benchmark) {
  for (int n = 50; n <= 50; n += 50) {
    for (int mode = 0; mode <= 2; ++mode) {
      benchmark->Args({mode, n});
    }
  }
}

BENCHMARK_REGISTER_F(Queens, probing_single_swap)->Ranges({{0, 2}, {5, 5000}});
BENCHMARK_REGISTER_F(Queens, probing_all_swap)
    ->Unit(benchmark::kMillisecond)
    ->Ranges({{0, 2}, {5, 1000}});

BENCHMARK_REGISTER_F(Queens, solve)->Apply(arguments);
//*/