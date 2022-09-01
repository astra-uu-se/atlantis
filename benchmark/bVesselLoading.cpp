#include <benchmark/benchmark.h>

#include <iostream>
#include <random>
#include <utility>
#include <vector>

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
  std::vector<VarId> west;
  std::vector<VarId> east;
  std::vector<VarId> north;
  std::vector<VarId> south;

  std::vector<VarId> violations;
  VarId totalViolation;

  std::vector<std::vector<Int>> bestPositions;
  std::vector<size_t> tabu;

  void SetUp(const ::benchmark::State& state) {
    engine = std::make_unique<PropagationEngine>();
    engine->open();
    engine->setPropagationMode(
        static_cast<PropagationEngine::PropagationMode>(state.range(0)));

    containerCount = state.range(1);

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
      west.push_back(engine->makeIntVar(0, 0, vesselWidth - m));
      east.push_back(engine->makeIntVar(0, m, vesselWidth));
      south.push_back(engine->makeIntVar(0, 0, vesselLength - m));
      north.push_back(engine->makeIntVar(0, m, vesselLength));
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
      VarId east0 =
          engine->makeIntView<IntOffsetView>(west[i], conWidth[i])->getId();
      VarId east1 =
          engine->makeIntView<IntOffsetView>(west[i], conLength[i])->getId();
      VarId north0 =
          engine->makeIntView<IntOffsetView>(south[i], conLength[i])->getId();
      VarId north1 =
          engine->makeIntView<IntOffsetView>(south[i], conWidth[i])->getId();

      // east[i] = west[i] + if orientation[i] == 0 then conWidth[i] else
      // conLength[i]
      engine->makeInvariant<IfThenElse>(orientation[i], east0, east1, east[i]);

      // north[i] = south[i] + if orientation[i] != 0 then conWidth[i] else
      // conLength[i]
      engine->makeInvariant<IfThenElse>(orientation[i], north0, north1,
                                        north[i]);
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

        VarId eastSep =
            sep == 0
                ? east[i]
                : engine->makeIntView<IntOffsetView>(east[i], sep)->getId();
        VarId westSep =
            sep == 0
                ? west[i]
                : engine->makeIntView<IntOffsetView>(west[i], -sep)->getId();
        VarId northSep =
            sep == 0
                ? north[i]
                : engine->makeIntView<IntOffsetView>(north[i], sep)->getId();
        VarId southSep =
            sep == 0
                ? south[i]
                : engine->makeIntView<IntOffsetView>(south[i], -sep)->getId();

        for (size_t j = i + 1; j < containerCount; ++j) {
          if (conClass[j] != c) {
            continue;
          }

          VarId isEastOf = engine->makeIntVar(0, 0, vesselLength + vesselWidth);
          VarId isWestOf = engine->makeIntVar(0, 0, vesselLength + vesselWidth);
          VarId isSouthOf =
              engine->makeIntVar(0, 0, vesselLength + vesselWidth);
          VarId isNorthOf =
              engine->makeIntVar(0, 0, vesselLength + vesselWidth);

          // isEastOf = east[i] + sep <= west[j]:
          engine->makeConstraint<LessEqual>(isEastOf, eastSep, west[j]);
          // isWestOf = east[j] <= west[i] - sep:
          engine->makeConstraint<LessEqual>(isWestOf, east[j], westSep);
          // isNorthOf = north[i] + sep <= south[j]:
          engine->makeConstraint<LessEqual>(isNorthOf, northSep, south[j]);
          // isSouthOf = north[j] <= south[i] - sep:
          engine->makeConstraint<LessEqual>(isSouthOf, north[j], southSep);

          violations.push_back(
              engine->makeIntVar(0, 0, vesselLength + vesselWidth));

          engine->makeConstraint<Exists>(
              violations.back(),
              std::vector<VarId>{isEastOf, isWestOf, isNorthOf, isSouthOf});
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
    engine->makeInvariant<Linear>(violations, totalViolation);
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
    west.clear();
    east.clear();
    north.clear();
    south.clear();

    violations.clear();
    for (std::vector<Int>& p : bestPositions) {
      p.clear();
    }
    tabu.clear();
  }
};

BENCHMARK_DEFINE_F(VesselLoading, rotate_rand_container)(benchmark::State& st) {
  auto distribution =
      std::uniform_int_distribution<>{0, (int)containerCount - 1};
  Int probes = 0;
  for (auto _ : st) {
    VarId v = orientation.at(distribution(gen));
    engine->beginMove();
    engine->setValue(v, 1 - engine->getCommittedValue(v));
    engine->endMove();

    engine->beginQuery();
    engine->query(totalViolation);
    engine->endQuery();

    ++probes;
  }

  st.counters["probes_per_second"] =
      benchmark::Counter(probes, benchmark::Counter::kIsRate);
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
            engine->setValue(west.at(c), newWest);
            engine->setValue(south.at(c), newSouth);
            engine->setValue(orientation.at(c), o);
            engine->endMove();

            // Probe
            engine->beginQuery();
            engine->query(totalViolation);
            engine->endQuery();

            ++probes;

            // Save improving move
            Int newValue = engine->getNewValue(totalViolation);
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
    engine->setValue(west[bestContainer], bestWest);
    engine->setValue(south[bestContainer], bestSouth);
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

///*
BENCHMARK_REGISTER_F(VesselLoading, rotate_rand_container)
    ->Unit(benchmark::kMillisecond)
    ->Ranges({{0, 2}, {20, 50}});

//*/

/*
BENCHMARK_REGISTER_F(VesselLoading, solve)
    ->Apply([](benchmark::internal::Benchmark* benchmark) {
      for (Int containerCount = 45; containerCount <= 50; containerCount += 5) {
        for (int mode = 2; mode <= 2; ++mode) {
          benchmark->Args({mode, containerCount});
        }
      }
    });

//*/