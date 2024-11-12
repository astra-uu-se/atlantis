#include <benchmark/benchmark.h>

#include <random>
#include <utility>
#include <vector>

#include "atlantis/propagation/invariants/ifThenElse.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/invariants/min.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"
#include "atlantis/propagation/violationInvariants/lessEqual.hpp"
#include "benchmark.hpp"

namespace atlantis::benchmark {

class VesselLoading : public ::benchmark::Fixture {
 public:
  std::shared_ptr<propagation::Solver> solver;
  std::random_device rd;
  std::mt19937 gen;

  const size_t vesselWidth = 50;   // Vessel vesselWidth
  const size_t vesselLength = 50;  // Vessel vesselLength
  const size_t classCount = 4;     // Num classes

  size_t containerCount{0};  // Num containers

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
  std::vector<propagation::VarViewId> orientation;
  std::vector<propagation::VarViewId> left;
  std::vector<propagation::VarViewId> bottom;

  propagation::VarViewId totalViolation{propagation::NULL_ID};

  std::uniform_int_distribution<size_t> indexDistr;
  std::uniform_int_distribution<Int> orientationDistr;
  std::vector<std::array<std::uniform_int_distribution<Int>, 2>> leftDistr;
  std::vector<std::array<std::uniform_int_distribution<Int>, 2>> bottomDistr;

  void SetUp(const ::benchmark::State& state) override {
    containerCount = state.range(0);

    solver = std::make_shared<propagation::Solver>();
    solver->open();
    setSolverMode(*solver, static_cast<int>(state.range(1)));

    gen = std::mt19937(rd());
    distClass = std::uniform_int_distribution<size_t>{0, classCount - 1};
    distDim = std::uniform_int_distribution<size_t>{2, 10};
    distSep = std::uniform_int_distribution<size_t>{0, 4};

    // conLength[i] = container i vesselLength:
    conLength.resize(containerCount);
    // conWidth[i]  = container i vesselWidth:
    conWidth.resize(containerCount);
    // conClass[i]  = container i class:
    std::vector<size_t> conClass(containerCount);

    orientation.clear();
    orientation.reserve(containerCount);

    left.clear();
    left.reserve(containerCount);

    std::vector<propagation::VarViewId> right;
    right.reserve(containerCount);

    bottom.clear();
    bottom.reserve(containerCount);
    std::vector<propagation::VarViewId> top;
    top.reserve(containerCount);

    // Create containerCount containers
    for (size_t i = 0; i < containerCount; ++i) {
      conLength[distDim(gen)];
      conWidth[distDim(gen)];
      conClass[distClass(gen)];

      // Create variables
      Int m = std::min(static_cast<Int>(conWidth[i]),
                       static_cast<Int>(conLength[i]));
      orientation.emplace_back(solver->makeIntVar(0, 0, 1));
      left.emplace_back(
          solver->makeIntVar(0, 0, static_cast<Int>(vesselWidth) - m));
      right.emplace_back(
          solver->makeIntVar(m, m, static_cast<Int>(vesselWidth)));
      bottom.emplace_back(
          solver->makeIntVar(0, 0, static_cast<Int>(vesselLength) - m));
      top.emplace_back(
          solver->makeIntVar(m, m, static_cast<Int>(vesselLength)));
    }

    // Create random min separation distance between classes.

    // seperations[c1][c2] = min distance between container class c1 and c2
    std::vector<std::vector<Int>> seperations(classCount,
                                              std::vector<Int>(classCount));
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
      propagation::VarViewId rightHorizontally =
          solver->makeIntView<propagation::IntOffsetView>(*solver, left[i],
                                                          conWidth[i]);
      propagation::VarViewId rightVertically =
          solver->makeIntView<propagation::IntOffsetView>(*solver, left[i],
                                                          conLength[i]);
      propagation::VarViewId topHorizontally =
          solver->makeIntView<propagation::IntOffsetView>(*solver, bottom[i],
                                                          conLength[i]);
      propagation::VarViewId topVertically =
          solver->makeIntView<propagation::IntOffsetView>(*solver, bottom[i],
                                                          conWidth[i]);

      // right[i] = left[i] + (if orientation[i] == 0 then conWidth[i] else
      // conLength[i] endif)
      solver->makeInvariant<propagation::IfThenElse>(
          *solver, right[i], orientation[i], rightHorizontally,
          rightVertically);

      // top[i] = bottom[i] + (if orientation[i] != 0 then conWidth[i] else
      // conLength[i] endif)
      solver->makeInvariant<propagation::IfThenElse>(
          *solver, top[i], orientation[i], topHorizontally, topVertically);
    }

    // Creating a number of static invariants that is quadratic in n, each with
    // a constant number of static input variables, resulting in a number of
    // static input variables that is quadratic in n.
    std::vector<propagation::VarViewId> violations{};
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

        propagation::VarViewId rightSep =
            sep == 0 ? right[i]
                     : solver->makeIntView<propagation::IntOffsetView>(
                           *solver, right[i], sep);
        propagation::VarViewId leftSep =
            sep == 0 ? left[i]
                     : solver->makeIntView<propagation::IntOffsetView>(
                           *solver, left[i], -sep);
        propagation::VarViewId topSep =
            sep == 0 ? top[i]
                     : solver->makeIntView<propagation::IntOffsetView>(
                           *solver, top[i], sep);
        propagation::VarViewId bottomSep =
            sep == 0 ? bottom[i]
                     : solver->makeIntView<propagation::IntOffsetView>(
                           *solver, bottom[i], -sep);

        for (size_t j = i + 1; j < containerCount; ++j) {
          if (conClass[j] != c) {
            continue;
          }

          propagation::VarViewId isRightOf = solver->makeIntVar(
              0, 0, static_cast<Int>(vesselLength + vesselWidth));
          propagation::VarViewId isLeftOf = solver->makeIntVar(
              0, 0, static_cast<Int>(vesselLength + vesselWidth));
          propagation::VarViewId isBelow = solver->makeIntVar(
              0, 0, static_cast<Int>(vesselLength + vesselWidth));
          propagation::VarViewId isAbove = solver->makeIntVar(
              0, 0, static_cast<Int>(vesselLength + vesselWidth));

          // isRightOf = (right[i] + sep <= left[j]):
          solver->makeViolationInvariant<propagation::LessEqual>(
              *solver, isRightOf, rightSep, left[j]);
          // isLeftOf = (right[j] <= left[i] - sep):
          solver->makeViolationInvariant<propagation::LessEqual>(
              *solver, isLeftOf, right[j], leftSep);
          // isAbove = (top[i] + sep <= bottom[j]):
          solver->makeViolationInvariant<propagation::LessEqual>(
              *solver, isAbove, topSep, bottom[j]);
          // isBelow = (top[j] <= bottom[i] - sep):
          solver->makeViolationInvariant<propagation::LessEqual>(
              *solver, isBelow, top[j], bottomSep);

          solver->makeInvariant<propagation::Min>(
              *solver,
              violations.emplace_back(solver->makeIntVar(
                  0, 0, static_cast<Int>(vesselLength + vesselWidth))),
              std::vector<propagation::VarViewId>{isRightOf, isLeftOf, isAbove,
                                                  isBelow},
              Int{0});
        }
      }
    }

    assert(violations.size() == containerCount * (containerCount - 1) / 2);

    // Add violations
    // Each container pair can bring vesselLength+vesselWidth violations.
    // and there are (containerCount*(containerCount-1)/2) pairs.
    totalViolation =
        solver->makeIntVar(0, 0,
                           (static_cast<Int>(containerCount) *
                            (static_cast<Int>(containerCount) - 1) / 2) *
                               (vesselLength + vesselWidth));
    solver->makeInvariant<propagation::Linear>(*solver, totalViolation,
                                               std::move(violations));
    solver->close();

    indexDistr = std::uniform_int_distribution<size_t>{0u, containerCount - 1};
    orientationDistr = std::uniform_int_distribution<Int>{0, 1};
    leftDistr.resize(containerCount);
    bottomDistr.resize(containerCount);
    for (size_t i = 0; i < containerCount; ++i) {
      leftDistr.at(i) = std::array<std::uniform_int_distribution<Int>, 2>{
          std::uniform_int_distribution<Int>{
              0, std::max<Int>(0, static_cast<Int>(vesselWidth) -
                                      static_cast<Int>(conWidth.at(i)))},
          std::uniform_int_distribution<Int>{
              0, std::max<Int>(
                     0, static_cast<Int>(vesselWidth) - conLength.at(i))}};
      bottomDistr.at(i) = std::array<std::uniform_int_distribution<Int>, 2>{
          std::uniform_int_distribution<Int>{
              0, std::max<Int>(0, static_cast<Int>(vesselLength) -
                                      static_cast<Int>(conLength.at(i)))},
          std::uniform_int_distribution<Int>{
              0, std::max<Int>(0, static_cast<Int>(vesselLength) -
                                      static_cast<Int>(conWidth.at(i)))}};
    }
  }

  void TearDown(const ::benchmark::State&) override {
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
  for ([[maybe_unused]] const auto& _ : st) {
    const size_t i = indexDistr(gen);
    assert(i < containerCount);
    const Int newOrientation = orientationDistr(gen);
    assert(0 <= newOrientation && newOrientation <= 1);
    const Int newLeft = leftDistr[i][newOrientation](gen);
    const Int newBottom = bottomDistr[i][newOrientation](gen);
    solver->beginMove();
    solver->setValue(orientation[i], newOrientation);
    solver->setValue(left[i], newLeft);
    solver->setValue(bottom[i], newBottom);
    solver->endMove();
    solver->beginProbe();
    solver->query(totalViolation);
    solver->endProbe();
    ++probes;
  }
  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

BENCHMARK_DEFINE_F(VesselLoading, solve)(::benchmark::State& st) {
  size_t it = 0;
  Int probes = 0;
  std::vector<size_t> tabu(containerCount, 0);
  Int tenure = 10;
  bool done = false;

  for ([[maybe_unused]] const auto& _ : st) {
    Int bestViol =
        static_cast<Int>((containerCount * (containerCount - 1) / 2) *
                         (vesselLength + vesselWidth));

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
        for (Int newWest =
                 static_cast<Int>(vesselWidth) -
                 static_cast<Int>(o == 0 ? conWidth[c] : conLength[c]);
             newWest >= 0; --newWest) {
          for (Int newSouth =
                   static_cast<Int>(vesselLength) -
                   static_cast<Int>(o == 0 ? conLength[c] : conWidth[c]);
               newSouth >= 0; --newSouth) {
            // Perform move
            solver->beginMove();
            solver->setValue(left.at(c), newWest);
            solver->setValue(bottom.at(c), newSouth);
            solver->setValue(orientation.at(c), o);
            solver->endMove();

            // Probe
            solver->beginProbe();
            solver->query(totalViolation);
            solver->endProbe();

            ++probes;

            // Save improving move
            Int newValue = solver->currentValue(totalViolation);
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
    solver->beginMove();
    solver->setValue(orientation[bestContainer], bestOrientation);
    solver->setValue(left[bestContainer], bestWest);
    solver->setValue(bottom[bestContainer], bestSouth);
    solver->endMove();

    solver->beginCommit();
    solver->query(totalViolation);
    solver->endCommit();

    tabu[bestContainer] = it + tenure;

    if (bestViol == 0) {
      done = true;
    }
  }

  // st.counters["it"]           = ::benchmark::Counter(it);
  // st.counters["it_per_s"]     = ::benchmark::Counter(it,
  // ::benchmark::Counter::kIsRate);
  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
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
