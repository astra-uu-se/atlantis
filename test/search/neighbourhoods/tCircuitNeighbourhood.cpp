#include <gtest/gtest.h>

#include "./testHelper.hpp"
#include "atlantis/search/neighbourhoods/circuitNeighbourhood.hpp"

namespace atlantis::testing {

using namespace atlantis::search::neighbourhoods;

class CircuitNeighbourhoodTest : public NeighbourhoodTestBase {
 public:
  std::shared_ptr<CircuitNeighbourhood> _neighbourhood;
  Int _offset = 1;

  std::vector<SearchVar> next;

  void SetUp() override {
    NeighbourhoodTestBase::SetUp();

    _solver->open();
    for (auto i = 0u; i < 4; ++i) {
      propagation::VarViewId var = _solver->makeIntVar(1, 1, 4);
      next.emplace_back(var, SearchDomain(1, 4));
    }

    _solver->close();

    _neighbourhood = std::make_shared<CircuitNeighbourhood>(
        std::vector<SearchVar>(next), _offset);
  }

  void expectHolds() {
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

TEST_F(CircuitNeighbourhoodTest, initialize) {
  initialize(*_neighbourhood);
  expectHolds();

  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    _neighbourhood->initialize(_random, *_assignment);
    expectHolds();
  }
}

TEST_F(CircuitNeighbourhoodTest, randomMove) {
  initialize(*_neighbourhood);
  expectHolds();

  for (auto i = 0; i < 1000; i++) {
    _neighbourhood->randomMove(_random, *_assignment);
    expectHolds();
  }
}

TEST_F(CircuitNeighbourhoodTest, commitIf) {
  initialize(*_neighbourhood);
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    commitIf(*_neighbourhood);
    expectHolds();
  }
}

}  // namespace atlantis::testing
