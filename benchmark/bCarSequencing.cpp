#include <benchmark/benchmark.h>

#include <cassert>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "atlantis/propagation/invariants/countConst.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/lessEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"
#include "atlantis/propagation/violationInvariants/equal.hpp"
#include "atlantis/propagation/violationInvariants/lessThan.hpp"
#include "benchmark.hpp"

namespace atlantis::benchmark {

class CarSequencing : public ::benchmark::Fixture {
 public:
  std::unique_ptr<propagation::Solver> solver;

  size_t numCars{0};
  const size_t numFeatures = 5;

  std::vector<size_t> classCount;
  std::vector<size_t> maxCarsInBlock;
  std::vector<size_t> blockSize;
  std::vector<std::vector<bool>> carData;
  std::vector<std::vector<Int>> carFeature;

  std::vector<propagation::VarId> sequence;
  propagation::VarId totalViolation;

  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<size_t> carDistribution;
  std::uniform_int_distribution<size_t> classDistribution;

  [[nodiscard]] inline size_t numClasses() const noexcept {
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
    std::uniform_int_distribution<size_t> featureDistribution(1,
                                                              numFeatures - 1);
    auto rng = std::default_random_engine{};
    carData = std::vector<std::vector<bool>>(
        numClasses(), std::vector<bool>(numFeatures, false));
    for (size_t c = 0; c < numClasses(); ++c) {
      const size_t classFeatureCount = featureDistribution(gen);
      for (size_t o = 0; o < classFeatureCount; ++o) {
        carData.at(c).at(o) = true;
      }
      std::shuffle(carData.at(c).begin(), carData.at(c).end(), rng);
    }

    for (size_t o = 0; o < numFeatures; ++o) {
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
    blockSize = std::vector<size_t>(numFeatures);
    std::iota(blockSize.begin(), blockSize.end(), 2);
    maxCarsInBlock = std::vector<size_t>(numFeatures);
    std::iota(maxCarsInBlock.begin(), maxCarsInBlock.end(), 1);
  }

  void initCarFeatures() {
    carFeature = std::vector<std::vector<Int>>(numFeatures);
    for (size_t o = 0; o < numFeatures; ++o) {
      carFeature.at(o) = std::vector<Int>(numCars);
      size_t i = 0;
      for (size_t c = 0; c < classCount.size(); ++c) {
        for (size_t j = 0; j < classCount.at(c); ++j) {
          carFeature.at(o).at(i) = carData.at(c).at(o) ? 1 : 0;
          ++i;
        }
      }
    }
  }

  void SetUp(const ::benchmark::State& state) override {
    solver = std::make_unique<propagation::Solver>();
    solver->open();

    numCars = state.range(0);
    setSolverMode(*solver, static_cast<int>(state.range(1)));

    gen = std::mt19937(rd());
    carDistribution = std::uniform_int_distribution<size_t>{0, numCars - 1};

    initClassCount();
    initCarData();
    initCarBlocks();
    initCarFeatures();

    assert(std::all_of(
        carFeature.begin(), carFeature.end(), [&](const std::vector<Int>& v) {
          return std::all_of(v.begin(), v.end(),
                             [&](const Int o) { return 0 <= o && o <= 1; });
        }));

    // introducing variables linear in numCars
    sequence = std::vector<propagation::VarId>(numCars);
    std::vector<propagation::VarId> violations{};
    // introducing variables linear in numCars
    violations.reserve(numFeatures * numCars);
    std::vector<propagation::VarId> featureElemSum{};
    // introducing variables linear in numCars
    featureElemSum.reserve(numFeatures * numCars);
    // introducing variables linear in numCars (numFeatures is a constant)
    std::vector<std::vector<propagation::VarId>> featureElem(numFeatures);
    for (size_t o = 0; o < numFeatures; ++o) {
      featureElem.at(o) =
          std::vector<propagation::VarId>(numCars, propagation::NULL_ID);
    }

    for (Int i = 0; i < static_cast<Int>(numCars); ++i) {
      sequence.at(i) = solver->makeIntVar(i, 0, static_cast<Int>(numCars) - 1);
    }

    for (size_t o = 0; o < numFeatures; ++o) {
      const size_t end = numCars - blockSize.at(o) + 1;
      for (size_t start = 0; start < end; ++start) {
        std::vector<propagation::VarId> featureElemRun(
            sequence.begin() + static_cast<Int>(start),
            sequence.begin() + static_cast<Int>(start + blockSize.at(o)));
        assert(featureElemRun.size() == blockSize.at(o));
        featureElemSum.emplace_back(
            solver->makeIntVar(0, 0, static_cast<Int>(blockSize.at(o))));
        // Introducing up to n invariants each with up to n static edges
        solver->makeInvariant<propagation::CountConst>(
            *solver, featureElemSum.back(), o, std::move(featureElemRun));
        // Introducing up to n invariants each with 2 static edges
        violations.emplace_back(
            solver->makeIntView<propagation::LessEqualConst>(
                *solver, featureElemSum.back(), maxCarsInBlock.at(o)));
      }
    }

    assert(violations.size() <= numFeatures * numCars);
    assert(featureElemSum.size() <= numFeatures * numCars);

    Int maxViol = 0;
    for (const propagation::VarId& viol : violations) {
      maxViol += solver->upperBound(viol);
    }
    totalViolation = solver->makeIntVar(0, 0, maxViol);
    // introducing one invariant with up to n edges
    solver->makeInvariant<propagation::Linear>(*solver, totalViolation,
                                               std::move(violations));

    solver->close();
  }

  void sanity() const {}

  void TearDown(const ::benchmark::State&) override {
    sequence.clear();
    classCount.clear();
    maxCarsInBlock.clear();
    blockSize.clear();
    for (auto& vec : carData) {
      vec.clear();
    }
    for (auto& vec : carFeature) {
      vec.clear();
    }
    carData.clear();
    carFeature.clear();
  }
};

BENCHMARK_DEFINE_F(CarSequencing, probe_single_swap)(::benchmark::State& st) {
  size_t probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    const size_t i = carDistribution(gen);
    assert(i < sequence.size());
    const size_t j = carDistribution(gen);
    assert(j < sequence.size());
    const Int oldI = solver->committedValue(sequence[i]);
    const Int oldJ = solver->committedValue(sequence[j]);
    // Perform random swap
    solver->beginMove();
    solver->setValue(sequence[i], oldJ);
    solver->setValue(sequence[j], oldI);
    solver->endMove();

    solver->beginProbe();
    solver->query(totalViolation);
    solver->endProbe();
    ++probes;
    assert(all_in_range(0, numCars - 1, [&](const size_t a) {
      return all_in_range(a + 1, numCars, [&](const size_t b) {
        return solver->committedValue(sequence.at(a)) !=
                   solver->committedValue(sequence.at(b)) &&
               solver->currentValue(sequence.at(a)) !=
                   solver->currentValue(sequence.at(b));
      });
    }));
  }
  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(CarSequencing, probe_all_swap)(::benchmark::State& st) {
  size_t probes = 0;
  for ([[maybe_unused]] const auto& _ : st) {
    for (size_t i = 0; i < static_cast<size_t>(numCars); ++i) {
      for (size_t j = i + 1; j < static_cast<size_t>(numCars); ++j) {
        const Int oldI = solver->committedValue(sequence[i]);
        const Int oldJ = solver->committedValue(sequence[j]);
        solver->beginMove();
        solver->setValue(sequence[i], oldJ);
        solver->setValue(sequence[j], oldI);
        solver->endMove();

        solver->beginProbe();
        solver->query(totalViolation);
        solver->endProbe();

        ++probes;
      }
    }
  }
  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

//*
BENCHMARK_REGISTER_F(CarSequencing, probe_single_swap)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments);

//*/
/*

BENCHMARK_REGISTER_F(CarSequencing, probe_all_swap)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments);

//*/
}  // namespace atlantis::benchmark
