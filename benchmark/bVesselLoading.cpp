#include <benchmark/benchmark.h>

#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "benchmark.hpp"
#include "propagation/constraints/lessEqual.hpp"
#include "propagation/invariants/elementVar.hpp"
#include "propagation/invariants/exists.hpp"
#include "propagation/invariants/ifThenElse.hpp"
#include "propagation/invariants/linear.hpp"
#include "propagation/propagationEngine.hpp"
#include "propagation/views/elementConst.hpp"
#include "propagation/views/intOffsetView.hpp"

namespace atlantis::benchmark {

class VesselLoading : public ::benchmark::Fixture {
 public:
  std::unique_ptr<propagation::PropagationEngine> engine;
  std::random_device rd;
  std::mt19937 gen;

  const size_t vesselWidth = 50;   // Vessel vesselWidth
  const size_t vesselLength = 50;  // Vessel vesselLength
  const size_t classCount = 4;     // Num classes

  size_t containerCount;  // Num containers

  std::uniform_int_distribution<size_t>
      distClass;  // dist for container classes
  std::uniform_int_distribution<size_t>
      distDim;  // dist for container dimensions
  std::uniform_int_distribution<size_t>
      distSep;  // dist for class min sep distance

  // conLength[i] = container i vesselLength:
  std::vector<size_t> conLength;
  // conWidth[i]  = container i vesselWidth:
  std::vector<size_t> conWidth;
  std::vector<propagation::VarId> orientation;
  std::vector<propagation::VarId> left;
  std::vector<propagation::VarId> bottom;

  propagation::VarId totalViolation;

  std::uniform_int_distribution<size_t> indexDistr;
  std::uniform_int_distribution<Int> orientationDistr;
  std::vector<std::array<std::uniform_int_distribution<Int>, 2>> leftDistr;
  std::vector<std::array<std::uniform_int_distribution<Int>, 2>> bottomDistr;

  void SetUp(const ::benchmark::State& state) {
    containerCount = state.range(0);

    engine = std::make_unique<propagation::PropagationEngine>();
    engine->open();
    setEngineModes(*engine, state.range(1));

    gen = std::mt19937(rd());
    distClass = std::uniform_int_distribution<size_t>{
        0, static_cast<size_t>(classCount - 1)};
    distDim = std::uniform_int_distribution<size_t>{2, 10};
    distSep = std::uniform_int_distribution<size_t>{0, 4};

    // conLength[i] = container i vesselLength:
    conLength.resize(containerCount);
    // conWidth[i]  = container i vesselWidth:
    conWidth.resize(containerCount);
    // conClass[i]  = container i class:
    std::vector<size_t> conClass(containerCount);

    orientation.resize(containerCount);
    left.resize(containerCount);
    std::vector<propagation::VarId> right(containerCount);
    bottom.resize(containerCount);
    std::vector<propagation::VarId> top(containerCount);

    // Create containerCount containers
    for (size_t i = 0; i < containerCount; ++i) {
      conLength[distDim(gen)];
      conWidth[distDim(gen)];
      conClass[distClass(gen)];

      // Create variables
      Int m = std::min(conWidth[i], conLength[i]);
      orientation[i] = engine->makeIntVar(0, 0, 1);
      left[i] = engine->makeIntVar(0, 0, vesselWidth - m);
      right[i] = engine->makeIntVar(m, m, vesselWidth);
      bottom[i] = engine->makeIntVar(0, 0, vesselLength - m);
      top[i] = engine->makeIntVar(m, m, vesselLength);
    }

    // Create random min separation distance between classes.

    // seperations[c1][c2] = min distance between container class c1 and c2
    std::vector<std::vector<int>> seperations(classCount,
                                              std::vector<int>(classCount));
    for (size_t c1 = 0; c1 < classCount; ++c1) {
      seperations[c1][c1] = 0;
      for (size_t c2 = c1 + 1; c2 < classCount; ++c2) {
        seperations[c1][c2] = distSep(gen);
        seperations[c2][c1] = seperations[c1][c2];
      }
    }

    // Creating 2 * n dynamic invariants, each with 1 static input variable and
    // 2 dynamic input variables, resulting in n static input variables and 2*n
    // dynamic input variables
    for (size_t i = 0; i < containerCount; ++i) {
      // orientation[i] = 0 <=> container i is positioned horizontally
      // orientation[i] = 1 <=> container i is positioned vertically
      propagation::VarId rightHorizontally =
          engine->makeIntView<propagation::IntOffsetView>(*engine, left[i],
                                                          conWidth[i]);
      propagation::VarId rightVertically =
          engine->makeIntView<propagation::IntOffsetView>(*engine, left[i],
                                                          conLength[i]);
      propagation::VarId topHorizontally =
          engine->makeIntView<propagation::IntOffsetView>(*engine, bottom[i],
                                                          conLength[i]);
      propagation::VarId topVertically =
          engine->makeIntView<propagation::IntOffsetView>(*engine, bottom[i],
                                                          conWidth[i]);

      // right[i] = left[i] + (if orientation[i] == 0 then conWidth[i] else
      // conLength[i] endif)
      engine->makeInvariant<propagation::IfThenElse>(
          *engine, right[i], orientation[i], rightHorizontally,
          rightVertically);

      // top[i] = bottom[i] + (if orientation[i] != 0 then conWidth[i] else
      // conLength[i] endif)
      engine->makeInvariant<propagation::IfThenElse>(
          *engine, top[i], orientation[i], topHorizontally, topVertically);
    }

    // Creating a number of static invariants that is quadratic in n, each with
    // a constant number of static input variables, resulting in a number of
    // static input variables that is quadratic in n.
    std::vector<propagation::VarId> violations{};
    violations.reserve(containerCount * (containerCount - 1) / 2);

    for (size_t i = 0; i < containerCount; ++i) {
      for (size_t c = 0; c < classCount; ++c) {
        bool exists = false;
        for (size_t j = i + 1; j < containerCount; ++j) {
          if (conClass[j] == c) {
            exists = true;
            break;
          }
        }
        if (!exists) {
          continue;
        }

        Int sep = seperations[conClass[i]][c];

        propagation::VarId rightSep =
            sep == 0 ? right[i]
                     : engine->makeIntView<propagation::IntOffsetView>(
                           *engine, right[i], sep);
        propagation::VarId leftSep =
            sep == 0 ? left[i]
                     : engine->makeIntView<propagation::IntOffsetView>(
                           *engine, left[i], -sep);
        propagation::VarId topSep =
            sep == 0 ? top[i]
                     : engine->makeIntView<propagation::IntOffsetView>(
                           *engine, top[i], sep);
        propagation::VarId bottomSep =
            sep == 0 ? bottom[i]
                     : engine->makeIntView<propagation::IntOffsetView>(
                           *engine, bottom[i], -sep);

        for (size_t j = i + 1; j < containerCount; ++j) {
          if (conClass[j] != c) {
            continue;
          }

          propagation::VarId isRightOf =
              engine->makeIntVar(0, 0, vesselLength + vesselWidth);
          propagation::VarId isLeftOf =
              engine->makeIntVar(0, 0, vesselLength + vesselWidth);
          propagation::VarId isBelow =
              engine->makeIntVar(0, 0, vesselLength + vesselWidth);
          propagation::VarId isAbove =
              engine->makeIntVar(0, 0, vesselLength + vesselWidth);

          // isRightOf = (right[i] + sep <= left[j]):
          engine->makeConstraint<propagation::LessEqual>(*engine, isRightOf,
                                                         rightSep, left[j]);
          // isLeftOf = (right[j] <= left[i] - sep):
          engine->makeConstraint<propagation::LessEqual>(*engine, isLeftOf,
                                                         right[j], leftSep);
          // isAbove = (top[i] + sep <= bottom[j]):
          engine->makeConstraint<propagation::LessEqual>(*engine, isAbove,
                                                         topSep, bottom[j]);
          // isBelow = (top[j] <= bottom[i] - sep):
          engine->makeConstraint<propagation::LessEqual>(*engine, isBelow,
                                                         top[j], bottomSep);

          engine->makeInvariant<propagation::Exists>(
              *engine,
              violations.emplace_back(
                  engine->makeIntVar(0, 0, vesselLength + vesselWidth)),
              std::vector<propagation::VarId>{isRightOf, isLeftOf, isAbove,
                                              isBelow});
        }
      }
    }

    assert(violations.size() == containerCount * (containerCount - 1) / 2);

    // Add violations
    // Each container pair can bring vesselLength+vesselWidth violations.
    // and there are (containerCount*(containerCount-1)/2) pairs.
    totalViolation =
        engine->makeIntVar(0, 0,
                           (containerCount * (containerCount - 1) / 2) *
                               (vesselLength + vesselWidth));
    engine->makeInvariant<propagation::Linear>(*engine, totalViolation,
                                               violations);
    engine->close();

    indexDistr = std::uniform_int_distribution<size_t>{0u, containerCount - 1};
    orientationDistr = std::uniform_int_distribution<Int>{0, 1};
    leftDistr.resize(containerCount);
    bottomDistr.resize(containerCount);
    for (size_t i = 0; i < containerCount; ++i) {
      leftDistr.at(i) = std::array<std::uniform_int_distribution<Int>, 2>{
          std::uniform_int_distribution<Int>{
              0, std::max<Int>(0, vesselWidth - conWidth.at(i))},
          std::uniform_int_distribution<Int>{
              0, std::max<Int>(0, vesselWidth - conLength.at(i))}};
      bottomDistr.at(i) = std::array<std::uniform_int_distribution<Int>, 2>{
          std::uniform_int_distribution<Int>{
              0, std::max<Int>(0, vesselLength - conLength.at(i))},
          std::uniform_int_distribution<Int>{
              0, std::max<Int>(0, vesselLength - conWidth.at(i))}};
    }
  }

  void TearDown(const ::benchmark::State&) {
    conLength.clear();
    conWidth.clear();
    orientation.clear();
    left.clear();
    bottom.clear();
  }
};

BENCHMARK_DEFINE_F(VesselLoading, probe_single_relocate)
(::benchmark::State& st) {
  size_t probes = 0;
  for (auto _ : st) {
    const size_t i = indexDistr(gen);
    assert(i < containerCount);
    const Int newOrientation = orientationDistr(gen);
    assert(0 <= newOrientation && newOrientation <= 1);
    const Int newLeft = leftDistr[i][newOrientation](gen);
    const Int newBottom = bottomDistr[i][newOrientation](gen);
    engine->beginMove();
    engine->setValue(orientation[i], newOrientation);
    engine->setValue(left[i], newLeft);
    engine->setValue(bottom[i], newBottom);
    engine->endMove();
    engine->beginProbe();
    engine->query(totalViolation);
    engine->endProbe();
    ++probes;
  }
  st.counters["probes_per_second"] =
      ::benchmark::Counter(probes, ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(VesselLoading, solve)(::benchmark::State& st) {
  size_t it = 0;
  Int probes = 0;
  std::vector<size_t> tabu(containerCount, 0);
  Int tenure = 10;
  bool done = false;

  for (auto _ : st) {
    Int bestViol = (containerCount * (containerCount - 1) / 2) *
                   (vesselLength + vesselWidth);

    size_t bestContainer = 0;
    Int bestWest = 0;
    Int bestSouth = 0;
    Int bestOrientation = 0;

    // Move loop: container, orientation, x,newSouth location
    for (size_t c = 0; c < containerCount; ++c) {
      if (tabu[c] > it) {
        continue;
      }

      for (Int o = 0; o <= 1; ++o) {
        for (Int newWest = vesselWidth - (o == 0 ? conWidth[c] : conLength[c]);
             newWest >= 0; --newWest) {
          for (Int newSouth =
                   vesselLength - (o == 0 ? conLength[c] : conWidth[c]);
               newSouth >= 0; --newSouth) {
            // Perform move
            engine->beginMove();
            engine->setValue(left.at(c), newWest);
            engine->setValue(bottom.at(c), newSouth);
            engine->setValue(orientation.at(c), o);
            engine->endMove();

            // Probe
            engine->beginProbe();
            engine->query(totalViolation);
            engine->endProbe();

            ++probes;

            // Save improving move
            Int newValue = engine->currentValue(totalViolation);
            if (newValue <= bestViol) {
              bestViol = newValue;
              bestContainer = c;
              bestOrientation = o;
              bestWest = newWest;
              bestSouth = newSouth;
            }
          }
        }
      }
    }

    // Commit move and mark as tabu
    engine->beginMove();
    engine->setValue(orientation[bestContainer], bestOrientation);
    engine->setValue(left[bestContainer], bestWest);
    engine->setValue(bottom[bestContainer], bestSouth);
    engine->endMove();

    engine->beginCommit();
    engine->query(totalViolation);
    engine->endCommit();

    tabu[bestContainer] = it + tenure;

    if (bestViol == 0) {
      done = true;
    }
  }

  // st.counters["it"]           = ::benchmark::Counter(it);
  // st.counters["it_per_s"]     = ::benchmark::Counter(it,
  // ::benchmark::Counter::kIsRate);
  st.counters["probes_per_second"] =
      ::benchmark::Counter(probes, ::benchmark::Counter::kIsRate);
  st.counters["solved"] = ::benchmark::Counter(done);
}

//*
BENCHMARK_REGISTER_F(VesselLoading, probe_single_relocate)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments);

//*/

/*
BENCHMARK_REGISTER_F(VesselLoading, solve)
    ->Apply([](::benchmark::internal::Benchmark* benchmark) {
      for (Int containerCount = 20; containerCount <= 20; containerCount += 5) {
        for (int mode = 2; mode <= 3; ++mode) {
          benchmark->Args({containerCount, mode});
        }
      }
    });

//*/
}  // namespace atlantis::benchmark