#include <gtest/gtest.h>

#include "./testHelper.hpp"
#include "atlantis/search/neighborhoods/circuitNeighborhood.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::testing {

using namespace atlantis::search::neighborhoods;

class CircuitNeighborhoodTest
    : public NeighborhoodTestBase<CircuitNeighborhood> {
 public:
  Int _offset = 1;

  std::vector<SearchVar> next;

  void SetUp() override {
    NeighborhoodTestBase::SetUp();

    _solver->open();
    for (auto i = 0u; i < 4; ++i) {
      propagation::VarViewId var = _solver->makeIntVar(1, 1, 4);
      next.emplace_back(var, std::make_shared<SearchDomain>(1, 4));
    }

    _solver->close();

    createNeighborhood(std::vector<SearchVar>(next), _offset);
  }

  void expectHolds() const {
    std::vector<bool> visited(next.size(), false);
    Int cur = 0;
    while (!visited.at(cur)) {
      visited.at(cur) = true;
      cur = _solver->committedValue(next.at(cur).solverId()) - _offset;
      EXPECT_GE(cur, 0);
      EXPECT_LT(cur, next.size());
    }

    for (size_t i = 0; i < next.size(); ++i) {
      EXPECT_TRUE(visited.at(i));
    }
  }
};

TEST_F(CircuitNeighborhoodTest, initialize) {
  initialize();
  expectHolds();

  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    _neighborhood->initialize(_random, *_assignment);
    expectHolds();
  }
}

TEST_F(CircuitNeighborhoodTest, randomMove) {
  initialize();
  expectHolds();

  for (auto i = 0; i < 1000; i++) {
    _neighborhood->randomMove(_random, *_assignment);
    expectHolds();
  }
}

TEST_F(CircuitNeighborhoodTest, commitIf) {
  initialize();
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    commitIf();
    expectHolds();
  }
}

}  // namespace atlantis::testing
