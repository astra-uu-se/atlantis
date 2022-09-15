#include <benchmark/benchmark.h>

#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "benchmark.hpp"
#include "constraints/lessEqual.hpp"
#include "core/propagationEngine.hpp"
#include "invariants/elementVar.hpp"
#include "invariants/exists.hpp"
#include "invariants/ifThenElse.hpp"
#include "invariants/linear.hpp"
#include "views/elementConst.hpp"
#include "views/intOffsetView.hpp"

class VesselLoading : public benchmark::Fixture {
 public:
  std::unique_ptr<PropagationEngine> engine;
  std::random_device rd;
  std::mt19937 gen;

  const Int vesselWidth = 50;   // Vessel vesselWidth
  const Int vesselLength = 50;  // Vessel vesselLength
  const Int classCount = 4;     // Num classes

  size_t containerCount;  // Num containers

  std::uniform_int_distribution<> distClass;  // dist for container classes
  std::uniform_int_distribution<> distDim;    // dist for container dimensions
  std::uniform_int_distribution<> distSep;    // dist for class min sep distance

  std::vector<Int> conLength;  // conLength[i] = container i vesselLength
  std::vector<Int> conWidth;   // conWidth[i]  = container i vesselWidth
  std::vector<Int> conClass;   // conClass[i]  = container i class
  std::vector<std::vector<int>>
      seperations;  // seperations[i][j] = min distance
                    // between container class i and j

  std::vector<VarId> orientation;
  std::vector<VarId> left;
  std::vector<VarId> right;
  std::vector<VarId> top;
  std::vector<VarId> bottom;

  std::vector<VarId> violations;
  VarId totalViolation;

  std::vector<std::vector<Int>> bestPositions;
  std::vector<size_t> tabu;

  void SetUp(const ::benchmark::State& state) {
    containerCount = state.range(0);

    engine = std::make_unique<PropagationEngine>();
    engine->open();
    setEngineModes(*engine, state.range(1));

    gen = std::mt19937(rd());
    distClass = std::uniform_int_distribution<>{0, (int)classCount - 1};
    distDim = std::uniform_int_distribution<>{2, 10};
    distSep = std::uniform_int_distribution<>{0, 4};

    // Create containerCount containers
    for (size_t c = 0; c < containerCount; ++c) {
      conLength.push_back(distDim(gen));
      conWidth.push_back(distDim(gen));
      conClass.push_back(distClass(gen));

      // Create variables
      Int m = std::min(conWidth[c], conLength[c]);
      orientation.push_back(engine->makeIntVar(0, 0, 1));
      left.push_back(engine->makeIntVar(0, 0, vesselWidth - m));
      right.push_back(engine->makeIntVar(m, m, vesselWidth));
      bottom.push_back(engine->makeIntVar(0, 0, vesselLength - m));
      top.push_back(engine->makeIntVar(m, m, vesselLength));
    }

    // Create random min separation distance between classes.
    seperations =
        std::vector<std::vector<int>>(classCount, std::vector<int>(classCount));
    for (Int i = 0; i < classCount; ++i) {
      seperations[i][i] = 0;
      for (Int j = i + 1; j < classCount; ++j) {
        seperations[i][j] = distSep(gen);
        seperations[j][i] = seperations[i][j];
      }
    }

    // Create invariants
    for (size_t i = 0; i < containerCount; ++i) {
      // orientation[i] = 0 <=> container i is positioned horizontally
      // orientation[i] = 1 <=> container i is positioned vertically
      VarId rightHorizontally =
          engine->makeIntView<IntOffsetView>(left[i], conWidth[i]);
      VarId rightVertically =
          engine->makeIntView<IntOffsetView>(left[i], conLength[i]);
      VarId topHorizontally =
          engine->makeIntView<IntOffsetView>(bottom[i], conLength[i]);
      VarId topVertically =
          engine->makeIntView<IntOffsetView>(bottom[i], conWidth[i]);

      // right[i] = left[i] + (if orientation[i] == 0 then conWidth[i] else
      // conLength[i] endif)
      engine->makeInvariant<IfThenElse>(right[i], orientation[i],
                                        rightHorizontally, rightVertically);

      // top[i] = bottom[i] + (if orientation[i] != 0 then conWidth[i] else
      // conLength[i] endif)
      engine->makeInvariant<IfThenElse>(top[i], orientation[i], topHorizontally,
                                        topVertically);
    }

    // No overlap between any container pair
    for (size_t i = 0; i < containerCount; ++i) {
      for (Int c = 0; c < classCount; ++c) {
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

        VarId rightSep =
            sep == 0 ? right[i]
                     : engine->makeIntView<IntOffsetView>(right[i], sep);
        VarId leftSep = sep == 0
                            ? left[i]
                            : engine->makeIntView<IntOffsetView>(left[i], -sep);
        VarId topSep =
            sep == 0 ? top[i] : engine->makeIntView<IntOffsetView>(top[i], sep);
        VarId bottomSep =
            sep == 0 ? bottom[i]
                     : engine->makeIntView<IntOffsetView>(bottom[i], -sep);

        for (size_t j = i + 1; j < containerCount; ++j) {
          if (conClass[j] != c) {
            continue;
          }

          VarId isRightOf =
              engine->makeIntVar(0, 0, vesselLength + vesselWidth);
          VarId isLeftOf = engine->makeIntVar(0, 0, vesselLength + vesselWidth);
          VarId isBelow = engine->makeIntVar(0, 0, vesselLength + vesselWidth);
          VarId isAbove = engine->makeIntVar(0, 0, vesselLength + vesselWidth);

          // isRightOf = right[i] + sep <= left[j]:
          engine->makeConstraint<LessEqual>(isRightOf, rightSep, left[j]);
          // isLeftOf = right[j] <= left[i] - sep:
          engine->makeConstraint<LessEqual>(isLeftOf, right[j], leftSep);
          // isAbove = top[i] + sep <= bottom[j]:
          engine->makeConstraint<LessEqual>(isAbove, topSep, bottom[j]);
          // isBelow = top[j] <= bottom[i] - sep:
          engine->makeConstraint<LessEqual>(isBelow, top[j], bottomSep);

          violations.push_back(
              engine->makeIntVar(0, 0, vesselLength + vesselWidth));

          engine->makeInvariant<Exists>(
              violations.back(),
              std::vector<VarId>{isRightOf, isLeftOf, isAbove, isBelow});
        }
      }
    }

    // Add violations
    // Each container pair can bring vesselLength+vesselWidth violations.
    // and there are (containerCount*(containerCount-1)/2) pairs.
    totalViolation =
        engine->makeIntVar(0, 0,
                           (containerCount * (containerCount - 1) / 2) *
                               (vesselLength + vesselWidth));
    engine->makeInvariant<Linear>(totalViolation, violations);
    engine->close();
  }

  void TearDown(const ::benchmark::State&) {
    conLength.clear();
    conWidth.clear();
    conClass.clear();
    for (std::vector<int>& s : seperations) {
      s.clear();
    }
    seperations.clear();
    orientation.clear();
    left.clear();
    right.clear();
    top.clear();
    bottom.clear();

    violations.clear();
    for (std::vector<Int>& p : bestPositions) {
      p.clear();
    }
    tabu.clear();
  }
};

BENCHMARK_DEFINE_F(VesselLoading, probe_single_relocate)(benchmark::State& st) {
  st.PauseTiming();
  auto indexDistr =
      std::uniform_int_distribution<size_t>{0u, containerCount - 1};
  auto rotationDistr = std::uniform_int_distribution<Int>{0, 1};
  std::vector<std::array<std::uniform_int_distribution<Int>, 2>> leftDistr;
  std::vector<std::array<std::uniform_int_distribution<Int>, 2>> rightDistr;
  for (size_t i = 0; i < containerCount; ++i) {
    leftDistr.emplace_back(std::array<std::uniform_int_distribution<Int>, 2>{
        std::uniform_int_distribution<Int>{
            0, std::max<Int>(0, vesselWidth - conWidth.at(i))},
        std::uniform_int_distribution<Int>{
            0, std::max<Int>(0, vesselWidth - conLength.at(i))}});
    rightDistr.emplace_back(std::array<std::uniform_int_distribution<Int>, 2>{
        std::uniform_int_distribution<Int>{
            0, std::max<Int>(0, vesselLength - conLength.at(i))},
        std::uniform_int_distribution<Int>{
            0, std::max<Int>(0, vesselLength - conWidth.at(i))}});
  }
  st.ResumeTiming();
  for (auto _ : st) {
    const size_t i = indexDistr(gen);
    assert(i < containerCount);
    const Int newRotation = rotationDistr(gen);
    const Int newLeft = leftDistr[i][newRotation](gen);
    const Int newBottom = rightDistr[i][newRotation](gen);
    engine->beginMove();
    engine->setValue(orientation[i], newRotation);
    engine->setValue(left[i], newLeft);
    engine->setValue(bottom[i], newBottom);
    engine->beginProbe();
    engine->query(totalViolation);
    engine->endProbe();
  }
}

BENCHMARK_DEFINE_F(VesselLoading, solve)(benchmark::State& st) {
  size_t it = 0;
  Int probes = 0;
  tabu.assign(containerCount, 0);
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

  // st.counters["it"]           = benchmark::Counter(it);
  // st.counters["it_per_s"]     = benchmark::Counter(it,
  // benchmark::Counter::kIsRate);
  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
  st.counters["solved"] = benchmark::Counter(done);
}

/*
static void arguments(benchmark::internal::Benchmark* benchmark) {
  for (int containerCount = 5; containerCount <= 50; containerCount += 5) {
    for (int mode = 0; mode <= 3; ++mode) {
      benchmark->Args({containerCount, mode});
    }
#ifndef NDEBUG
    return;
#endif
  }
}

BENCHMARK_REGISTER_F(VesselLoading, probe_single_relocate)
    ->Apply(arguments)
    ->Unit(benchmark::kMillisecond);

//*/

/*
BENCHMARK_REGISTER_F(VesselLoading, solve)
    ->Apply([](benchmark::internal::Benchmark* benchmark) {
      for (Int containerCount = 20; containerCount <= 20; containerCount += 5) {
        for (int mode = 2; mode <= 3; ++mode) {
          benchmark->Args({containerCount, mode});
        }
      }
    });

//*/