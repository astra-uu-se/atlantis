#pragma once
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/search/annealer.hpp"
#include "atlantis/search/annealing/annealingSchedule.hpp"
#include "atlantis/search/cost.hpp"
#include "atlantis/search/iAssignment.hpp"
#include "atlantis/search/neighborhoods/neighborhood.hpp"
#include "atlantis/search/randomProvider.hpp"

namespace atlantis::testing {

using namespace atlantis::search;
using namespace atlantis::search::neighborhoods;

class SimpleAssignment : public virtual IAssignment {
 public:
  std::shared_ptr<propagation::Solver> _solver;

  SimpleAssignment(std::shared_ptr<propagation::Solver> solver)
      : _solver(solver) {}

  Cost initialize(RandomProvider&) override {
    return Cost(0, 0, ObjectiveDirection::NONE);
  }

  void initialize(Neighborhood& neighborhood, RandomProvider& random) {
    _solver->beginMove();
    neighborhood.initialize(random, *this);
    _solver->endMove();
    _solver->beginCommit();
    _solver->endCommit();
  }

  Cost performProbe(RandomProvider&) override {
    return Cost(0, 0, ObjectiveDirection::NONE);
  }

  void commitLastProbe() override {}

  Int currentValue(propagation::VarViewId var) const override {
    return _solver->currentValue(var);
  }

  [[nodiscard]] Int committedValue(propagation::VarViewId var) const override {
    return _solver->committedValue(var);
  }

  [[nodiscard]] bool satisfiesConstraints() const override { return true; }

  [[nodiscard]] bool objectiveIsOptimal() const override { return true; }

  void set(propagation::VarId searchVarId, Int val) override {
    return _solver->setValue(searchVarId, val);
  }

  [[nodiscard]] const std::vector<propagation::VarId>& searchVars()
      const override {
    return _solver->searchVars();
  }

  [[nodiscard]] Timestamp currentTimestamp() const override {
    return _solver->currentTimestamp();
  }

  [[nodiscard]] ObjectiveDirection objectiveDirection() const override {
    return ObjectiveDirection::NONE;
  }
};

class NeighborhoodTestBase : public ::testing::Test {
 public:
  std::shared_ptr<propagation::Solver> _solver;
  std::shared_ptr<SimpleAssignment> _assignment;
  RandomProvider _random{123456789};

  void SetUp() override {
    _solver = std::make_shared<propagation::Solver>();
    _assignment = std::make_shared<SimpleAssignment>(_solver);
  }

  void initialize(Neighborhood& neighborhood) {
    _solver->beginMove();
    neighborhood.initialize(_random, *_assignment);
    _solver->endMove();
    _solver->beginCommit();
    _solver->endCommit();
  }

  void randomMove(Neighborhood& neighborhood) {
    _solver->beginMove();
    neighborhood.randomMove(_random, *_assignment);
    _solver->endMove();
    _solver->beginProbe();
    _solver->endProbe();
  }

  void commitIf(Neighborhood& neighborhood) {
    _solver->beginMove();
    neighborhood.randomMove(_random, *_assignment);
    _solver->endMove();
    neighborhood.commitIf(*_assignment);
    _solver->beginCommit();
    _solver->endCommit();
  }
};

}  // namespace atlantis::testing