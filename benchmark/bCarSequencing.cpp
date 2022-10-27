#include <benchmark/benchmark.h>

#include <cassert>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "benchmark.hpp"
#include "constraints/allDifferent.hpp"
#include "constraints/equal.hpp"
#include "constraints/lessThan.hpp"
#include "core/propagationEngine.hpp"
#include "invariants/linear.hpp"
#include "views/elementConst.hpp"
#include "views/lessEqualConst.hpp"

class CarSequencing : public benchmark::Fixture {
 public:
  std::unique_ptr<PropagationEngine> engine;

  size_t numCars;
  size_t numClasses;
  size_t numOptions;

  std::vector<VarId> sequence;
  VarId totalViolation;

  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<size_t> carDistribution;

  void SetUp(const ::benchmark::State& state) {
    engine = std::make_unique<PropagationEngine>();
    engine->open();

    numCars = state.range(0);
    setEngineModes(*engine, state.range(1));
    numOptions = 5;
    numClasses = numCars / 2;

    gen = std::mt19937(rd());
    carDistribution = std::uniform_int_distribution<size_t>{0, numCars - 1};

    // Give each car a class:
    std::vector<size_t> classCount(numClasses, 1);
    std::uniform_int_distribution<> classDistribution(0, numClasses - 1);
    for (size_t i = numClasses; i < numCars; ++i) {
      ++classCount.at(classDistribution(gen));
    }

    // Set up the options for the classes:
    std::uniform_int_distribution<size_t> optionDistribution(1, numOptions - 1);
    auto rng = std::default_random_engine{};
    std::vector<std::vector<Int>> carData(numClasses);
    for (size_t c = 0; c < numClasses; ++c) {
      carData.at(c) = std::vector<Int>(numOptions, 0);
      const size_t classOptionCount = optionDistribution(gen);
      for (size_t o = 0; o < classOptionCount; ++o) {
        carData.at(c).at(o) = 1;
      }
      std::shuffle(carData.at(c).begin(), carData.at(c).end(), rng);
    }

    for (size_t o = 0; o < numOptions; ++o) {
      bool found = false;
      for (size_t c = 0; c < numClasses; ++c) {
        if (carData.at(c).at(o) == 1) {
          found = true;
          break;
        }
      }
      if (!found) {
        carData.at(classDistribution(gen)).at(o) = 1;
      }
    }

    std::vector<Int> maxCarsInBlock(numOptions);
    std::vector<Int> blockSize(numOptions);
    for (size_t o = 0; o < numOptions; ++o) {
      maxCarsInBlock.at(o) = 1 + carDistribution(gen);
      blockSize.at(o) = 1 + carDistribution(gen);
    }

    std::vector<std::vector<Int>> carOption(numOptions);
    for (size_t o = 0; o < numOptions; ++o) {
      carOption.at(o) = std::vector<Int>(numCars);
      size_t i = 0;
      for (size_t c = 0; c < classCount.size(); ++c) {
        for (size_t j = 0; j < classCount.at(c); ++j) {
          carOption.at(o).at(i) = carData.at(c).at(o);
          ++i;
        }
      }
    }
    assert(std::all_of(
        carOption.begin(), carOption.end(), [&](const std::vector<Int>& v) {
          return std::all_of(v.begin(), v.end(),
                             [&](const Int o) { return 0 <= o && o <= 1; });
        }));

    sequence = std::vector<VarId>();
    for (size_t i = 0; i < numCars; ++i) {
      sequence.emplace_back(engine->makeIntVar(i, 0, numCars - 1));
    }

    std::vector<VarId> violations;
    engine->makeConstraint<AllDifferent>(
        *engine, violations.emplace_back(engine->makeIntVar(0, 0, numCars - 1)),
        sequence);

    for (size_t o = 0; o < numOptions; ++o) {
      for (Int start = 0;
           start < static_cast<Int>(numCars) - blockSize.at(o) + 1; ++start) {
        std::vector<VarId> optionRun;
        for (Int car = start; car < start + blockSize.at(o) - 1; ++car) {
          optionRun.emplace_back(engine->makeIntView<ElementConst>(
              *engine, sequence.at(car), carOption.at(o)));
        }
        VarId sum = engine->makeIntVar(0, 0, blockSize.at(o));
        engine->makeInvariant<Linear>(*engine, sum, optionRun);
        violations.emplace_back(engine->makeIntView<LessEqualConst>(
            *engine, sum, maxCarsInBlock.at(o)));
      }
    }

    // differences must be unique
    Int maxViol = 0;
    for (const VarId viol : violations) {
      maxViol += engine->upperBound(viol);
    }
    totalViolation = engine->makeIntVar(0, 0, maxViol);
    engine->makeInvariant<Linear>(*engine, totalViolation, violations);

    engine->close();
  }

  void TearDown(const ::benchmark::State&) { sequence.clear(); }
};

BENCHMARK_DEFINE_F(CarSequencing, probe_single_swap)(benchmark::State& st) {
  size_t probes = 0;
  for (auto _ : st) {
    const size_t i = carDistribution(gen);
    assert(i < sequence.size());
    const size_t j = carDistribution(gen);
    assert(j < sequence.size());
    const Int oldI = engine->committedValue(sequence[i]);
    const Int oldJ = engine->committedValue(sequence[j]);
    // Perform random swap
    engine->beginMove();
    engine->setValue(sequence[i], oldJ);
    engine->setValue(sequence[j], oldI);
    engine->endMove();

    engine->beginProbe();
    engine->query(totalViolation);
    engine->endProbe();
    ++probes;
  }
  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(CarSequencing, probe_all_swap)(benchmark::State& st) {
  size_t probes = 0;
  for (auto _ : st) {
    for (size_t i = 0; i < static_cast<size_t>(numCars); ++i) {
      for (size_t j = i + 1; j < static_cast<size_t>(numCars); ++j) {
        const Int oldI = engine->committedValue(sequence[i]);
        const Int oldJ = engine->committedValue(sequence[j]);
        engine->beginMove();
        engine->setValue(sequence[i], oldJ);
        engine->setValue(sequence[j], oldI);
        engine->endMove();

        engine->beginProbe();
        engine->query(totalViolation);
        engine->endProbe();

        ++probes;
      }
    }
  }
  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
}

/*

static void arguments(benchmark::internal::Benchmark* benchmark) {
  for (int numCars = 20; numCars <= 150; numCars += 20) {
    for (int mode = 0; mode <= 3; ++mode) {
      benchmark->Args({numCars, mode});
    }
#ifndef NDEBUG
    return;
#endif
  }
}

BENCHMARK_REGISTER_F(CarSequencing, probe_single_swap)
    ->Unit(benchmark::kMillisecond)
    ->Apply(arguments);

//*/
/*

BENCHMARK_REGISTER_F(CarSequencing, probe_all_swap)
    ->Unit(benchmark::kMillisecond)
    ->Apply(arguments);

//*/