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
  const size_t numOptions = 5;

  std::vector<size_t> classCount;
  std::vector<size_t> maxCarsInBlock;
  std::vector<size_t> blockSize;
  std::vector<std::vector<bool>> carData;
  std::vector<std::vector<Int>> carOption;

  std::vector<VarId> sequence;
  VarId totalViolation;

  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<size_t> carDistribution;
  std::uniform_int_distribution<size_t> classDistribution;

  inline size_t numClasses() const noexcept {
    return numCars == 0 ? 0 : numCars / 2;
  }

  void initClassCount() {
    classCount = std::vector<size_t>(numClasses(), 1);
    classDistribution =
        std::uniform_int_distribution<size_t>(0, numClasses() - 1);
    for (size_t i = numClasses(); i < numCars; ++i) {
      ++classCount[classDistribution(gen)];
    }
  }

  void initCarData() {
    std::uniform_int_distribution<size_t> optionDistribution(1, numOptions - 1);
    auto rng = std::default_random_engine{};
    carData = std::vector<std::vector<bool>>(
        numClasses(), std::vector<bool>(numOptions, false));
    for (size_t c = 0; c < numClasses(); ++c) {
      const size_t classOptionCount = optionDistribution(gen);
      for (size_t o = 0; o < classOptionCount; ++o) {
        carData.at(c).at(o) = true;
      }
      std::shuffle(carData.at(c).begin(), carData.at(c).end(), rng);
    }

    for (size_t o = 0; o < numOptions; ++o) {
      bool found = false;
      for (size_t c = 0; c < numClasses(); ++c) {
        if (carData.at(c).at(o) == 1) {
          found = true;
          break;
        }
      }
      if (!found) {
        carData.at(classDistribution(gen)).at(o) = true;
      }
    }
  }

  void initCarBlocks() {
    maxCarsInBlock = std::vector<size_t>(numOptions);
    blockSize = std::vector<size_t>(numOptions);
    for (size_t o = 0; o < numOptions; ++o) {
      maxCarsInBlock.at(o) = 1 + carDistribution(gen);
      blockSize.at(o) = 1 + carDistribution(gen);
    }
  }

  void initCarOptions() {
    carOption = std::vector<std::vector<Int>>(numOptions);
    for (size_t o = 0; o < numOptions; ++o) {
      carOption.at(o) = std::vector<Int>(numCars);
      size_t i = 0;
      for (size_t c = 0; c < classCount.size(); ++c) {
        for (size_t j = 0; j < classCount.at(c); ++j) {
          carOption.at(o).at(i) = carData.at(c).at(o) ? 1 : 0;
          ++i;
        }
      }
    }
  }

  void SetUp(const ::benchmark::State& state) {
    engine = std::make_unique<PropagationEngine>();
    engine->open();

    numCars = state.range(0);
    setEngineModes(*engine, state.range(1));

    gen = std::mt19937(rd());
    carDistribution = std::uniform_int_distribution<size_t>{0, numCars - 1};

    initClassCount();
    initCarData();
    initCarBlocks();
    initCarOptions();

    assert(std::all_of(
        carOption.begin(), carOption.end(), [&](const std::vector<Int>& v) {
          return std::all_of(v.begin(), v.end(),
                             [&](const Int o) { return 0 <= o && o <= 1; });
        }));

    // introducing variables linear in numCars
    sequence = std::vector<VarId>(numCars);
    std::vector<VarId> violations{};
    // introducing variables linear in numCars
    violations.reserve(numOptions * numCars);
    std::vector<VarId> optionElemSum{};
    // introducing variables linear in numCars
    optionElemSum.reserve(numOptions * numCars);
    // introducing variables linear in numCars (numOptions is a constant)
    std::vector<std::vector<VarId>> optionElem(numOptions);
    for (size_t o = 0; o < numOptions; ++o) {
      optionElem.at(o) = std::vector<VarId>(numCars, NULL_ID);
    }

    for (size_t i = 0; i < numCars; ++i) {
      sequence.at(i) = engine->makeIntVar(i, 0, numCars - 1);
    }

    for (size_t o = 0; o < numOptions; ++o) {
      for (size_t start = 0; start < numCars - blockSize.at(o) + 1; ++start) {
        std::vector<VarId> optionElemRun{};
        optionElemRun.reserve(blockSize.at(o) - 1);
        for (size_t car = start; car < start + blockSize.at(o) && car < numCars;
             ++car) {
          if (optionElem.at(o).at(car) == NULL_ID) {
            optionElem.at(o).at(car) = engine->makeIntView<ElementConst>(
                *engine, sequence.at(car), carOption.at(o));
          }
          optionElemRun.emplace_back(optionElem.at(o).at(car));
        }
        assert(optionElemRun.size() == blockSize.at(o));
        optionElemSum.emplace_back(engine->makeIntVar(0, 0, blockSize.at(o)));
        // Introducing up to n invariants each with up to n static edges
        engine->makeInvariant<Linear>(*engine, optionElemSum.back(),
                                      optionElemRun);
        // Introducing up to n invariants each with 2 static edges
        violations.emplace_back(engine->makeIntView<LessEqualConst>(
            *engine, optionElemSum.back(), maxCarsInBlock.at(o)));
      }
    }

    assert(violations.size() <= numOptions * numCars);
    assert(optionElemSum.size() <= numOptions * numCars);

    Int maxViol = 0;
    for (const VarId viol : violations) {
      maxViol += engine->upperBound(viol);
    }
    totalViolation = engine->makeIntVar(0, 0, maxViol);
    // introducing one invariant with up to n edges
    engine->makeInvariant<Linear>(*engine, totalViolation, violations);

    engine->close();
  }

  void sanity() const {}

  void TearDown(const ::benchmark::State&) {
    sequence.clear();
    classCount.clear();
    maxCarsInBlock.clear();
    blockSize.clear();
    for (auto& vec : carData) {
      vec.clear();
    }
    for (auto& vec : carOption) {
      vec.clear();
    }
    carData.clear();
    carOption.clear();
  }
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
    assert(all_in_range(0, numCars - 1, [&](const size_t a) {
      return all_in_range(a + 1, numCars, [&](const size_t b) {
        return engine->committedValue(sequence.at(a)) !=
                   engine->committedValue(sequence.at(b)) &&
               engine->currentValue(sequence.at(a)) !=
                   engine->currentValue(sequence.at(b));
      });
    }));
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

//*
BENCHMARK_REGISTER_F(CarSequencing, probe_single_swap)
    ->Unit(benchmark::kMillisecond)
    ->Apply(defaultArguments);

//*/
/*

BENCHMARK_REGISTER_F(CarSequencing, probe_all_swap)
    ->Unit(benchmark::kMillisecond)
    ->Apply(defaultArguments);

//*/