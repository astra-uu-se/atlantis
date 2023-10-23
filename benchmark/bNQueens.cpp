#include <benchmark/benchmark.h>

#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "benchmark.hpp"
#include "propagation/constraints/allDifferent.hpp"
#include "propagation/invariants/linear.hpp"
#include "propagation/propagationEngine.hpp"
#include "propagation/views/intOffsetView.hpp"

namespace atlantis::benchmark {

class Queens : public ::benchmark::Fixture {
 public:
  std::unique_ptr<propagation::PropagationEngine> engine;
  std::vector<propagation::VarId> queens;
  std::vector<propagation::VarId> q_offset_plus;
  std::vector<propagation::VarId> q_offset_minus;
  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<Int> distribution;
  Int n;

  propagation::VarId violation1 = propagation::NULL_ID;
  propagation::VarId violation2 = propagation::NULL_ID;
  propagation::VarId violation3 = propagation::NULL_ID;
  propagation::VarId totalViolation = propagation::NULL_ID;

  void SetUp(const ::benchmark::State& state) override {
    engine = std::make_unique<propagation::PropagationEngine>();
    n = state.range(0);
    if (n < 0) {
      throw std::runtime_error("n must be non-negative.");
    }

    engine->open();
    setEngineModes(*engine, state.range(1));
    // the total number of variables is linear in n
    queens = std::vector<propagation::VarId>(n);
    q_offset_minus = std::vector<propagation::VarId>(n);
    q_offset_plus = std::vector<propagation::VarId>(n);

    for (Int i = 0; i < n; ++i) {
      queens.at(i) = engine->makeIntVar(i, 0, n - 1);
      q_offset_minus.at(i) = engine->makeIntView<propagation::IntOffsetView>(
          *engine, queens.at(i), -i);
      q_offset_plus.at(i) = engine->makeIntView<propagation::IntOffsetView>(
          *engine, queens.at(i), i);
    }

    violation1 = engine->makeIntVar(0, 0, n);
    violation2 = engine->makeIntVar(0, 0, n);
    violation3 = engine->makeIntVar(0, 0, n);

    // 3 invariants, each having taking n static input variables
    engine->makeConstraint<propagation::AllDifferent>(*engine, violation1,
                                                      queens);
    engine->makeConstraint<propagation::AllDifferent>(*engine, violation2,
                                                      q_offset_minus);
    engine->makeConstraint<propagation::AllDifferent>(*engine, violation3,
                                                      q_offset_plus);

    totalViolation = engine->makeIntVar(0, 0, 3 * n);

    engine->makeInvariant<propagation::Linear>(
        *engine, totalViolation,
        std::vector<propagation::VarId>{violation1, violation2, violation3});

    engine->close();

    gen = std::mt19937(rd());

    distribution = std::uniform_int_distribution<Int>{0, n - 1};
  }

  void TearDown(const ::benchmark::State&) override {
    queens.clear();
    q_offset_minus.clear();
    q_offset_plus.clear();
  }

  std::string instanceToString() {
    std::string str = "Queens: ";
    for (auto q : queens) {
      str += std::to_string(engine->committedValue(q)) + ", ";
    }
    return str;
  }

  inline bool sanity() const {
    return all_in_range(0, n - 1, [&](const size_t i) {
      return all_in_range(i + 1, n, [&](const size_t j) {
        return engine->committedValue(queens.at(i)) !=
                   engine->committedValue(queens.at(j)) &&
               engine->currentValue(queens.at(i)) !=
                   engine->currentValue(queens.at(j));
      });
    });
  }
};

BENCHMARK_DEFINE_F(Queens, probe_single_swap)(::benchmark::State& st) {
  size_t probes = 0;
  for (auto _ : st) {
    const size_t i = distribution(gen);
    assert(i < queens.size());
    const size_t j = distribution(gen);
    assert(j < queens.size());
    const Int oldI = engine->committedValue(queens[i]);
    const Int oldJ = engine->committedValue(queens[j]);
    // Perform random swap
    engine->beginMove();
    engine->setValue(queens[i], oldJ);
    engine->setValue(queens[j], oldI);
    engine->endMove();

    engine->beginProbe();
    engine->query(totalViolation);
    engine->endProbe();
    ++probes;
    assert(sanity());
  }
  st.counters["probes_per_second"] =
      ::benchmark::Counter(probes, ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(Queens, probe_all_swap)(::benchmark::State& st) {
  int probes = 0;
  for (auto _ : st) {
    for (size_t i = 0; i < static_cast<size_t>(n); ++i) {
      for (size_t j = i + 1; j < static_cast<size_t>(n); ++j) {
        const Int oldI = engine->committedValue(queens[i]);
        const Int oldJ = engine->committedValue(queens[j]);
        engine->beginMove();
        engine->setValue(queens[i], oldJ);
        engine->setValue(queens[j], oldI);
        engine->endMove();

        engine->beginProbe();
        engine->query(totalViolation);
        engine->endProbe();

        ++probes;
        assert(sanity());
      }
    }
  }
  st.counters["probes_per_second"] =
      ::benchmark::Counter(probes, ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(Queens, solve)(::benchmark::State& st) {
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
          const Int oldI = engine->committedValue(queens[i]);
          const Int oldJ = engine->committedValue(queens[j]);
          engine->beginMove();
          engine->setValue(queens[i], oldJ);
          engine->setValue(queens[j], oldI);
          engine->endMove();

          engine->beginProbe();
          engine->query(totalViolation);
          engine->endProbe();

          ++probes;
          assert(sanity());

          Int newValue = engine->currentValue(totalViolation);
          if (newValue <= bestViol) {
            bestViol = newValue;
            bestI = i;
            bestJ = j;
          }
        }
      }

      const Int oldI = engine->committedValue(queens[bestI]);
      const Int oldJ = engine->committedValue(queens[bestJ]);
      engine->beginMove();
      engine->setValue(queens[bestI], oldJ);
      engine->setValue(queens[bestJ], oldI);
      engine->endMove();

      engine->beginCommit();
      engine->query(totalViolation);
      engine->endCommit();

      tabu[bestI] = it + tenure;
      tabu[bestJ] = it + tenure;
      if (bestViol == 0) {
        done = true;
      }
      ++it;
    }
  }

  st.counters["it"] = ::benchmark::Counter(it);

  st.counters["it_per_s"] =
      ::benchmark::Counter(it, ::benchmark::Counter::kIsRate);
  st.counters["probes_per_s"] =
      ::benchmark::Counter(probes, ::benchmark::Counter::kIsRate);
  st.counters["solved"] = ::benchmark::Counter(done);
  logDebug(instanceToString());
}

//*
BENCHMARK_REGISTER_F(Queens, probe_single_swap)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments);

/*
BENCHMARK_REGISTER_F(Queens, probe_all_swap)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments);

/*
BENCHMARK_REGISTER_F(Queens, solve)->Apply(defaultArguments);
//*/
}  // namespace atlantis::benchmark