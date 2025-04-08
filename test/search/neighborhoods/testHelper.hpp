#pragma once
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/search/annealing/annealingSchedule.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/randomProvider.hpp"

namespace atlantis::testing {

using namespace atlantis::search;
using namespace atlantis::search::neighborhoods;

template <class N>
class NeighborhoodTestBase : public ::testing::Test {
 public:
  std::shared_ptr<propagation::Solver> _solver;
  std::shared_ptr<N> _neighborhood{nullptr};
  std::shared_ptr<Assignment> _assignment{nullptr};
  RandomProvider _random{123456789};

  void SetUp() override { _solver = std::make_shared<propagation::Solver>(); }

  template <typename... Args>
  void createNeighborhood(Args&&... args) {
    EXPECT_EQ(_neighborhood, nullptr);
    _neighborhood = std::make_shared<N>(std::forward<Args>(args)...);
    _assignment = std::make_shared<Assignment>(
        *_solver, *_neighborhood, propagation::NULL_ID, propagation::NULL_ID,
        ObjectiveDirection::NONE, 0);
  }

  void initialize() {
    if (_solver->isOpen()) {
      _solver->close();
    }
    _solver->beginMove();
    _neighborhood->initialize(_random, *_assignment);
    _solver->endMove();
    _solver->beginCommit();
    _solver->endCommit();
  }

  void randomMove() {
    if (_solver->isOpen()) {
      _solver->close();
    }
    _solver->beginMove();
    _neighborhood->randomMove(_random, *_assignment);
    _solver->endMove();
    _solver->beginProbe();
    _solver->endProbe();
  }

  void commitIf() {
    if (_solver->isOpen()) {
      _solver->close();
    }
    _solver->beginMove();
    _neighborhood->randomMove(_random, *_assignment);
    _solver->endMove();
    _neighborhood->commitIf(*_assignment);
    _solver->beginCommit();
    _solver->endCommit();
  }
};

}  // namespace atlantis::testing