#include <gtest/gtest.h>

#include "./testHelper.hpp"
#include "atlantis/search/neighborhoods/randomNeighborhood.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::testing {

using namespace atlantis::search::neighborhoods;

class RandomNeighborhoodTest : public NeighborhoodTestBase<RandomNeighborhood> {
 public:
  Int _offset = 1;

  std::vector<SearchVar> _vars;

  void SetUp() override {
    NeighborhoodTestBase::SetUp();

    _solver->open();
    for (auto i = 0u; i < 4; ++i) {
      propagation::VarViewId var = _solver->makeIntVar(1, 1, 4);
      _vars.emplace_back(var, std::make_shared<SearchDomain>(1, 4));
    }

    _solver->close();

    createNeighborhood(std::vector<SearchVar>(_vars));
  }

  size_t expectHolds() const {
    size_t numModified{0};
    for (const auto& var : _vars) {
      const Int curVal = _solver->currentValue(var.solverId());
      const Int comVal = _solver->committedValue(var.solverId());
      EXPECT_TRUE(var.domain()->contains(curVal));
      EXPECT_TRUE(var.domain()->contains(comVal));
      if (curVal != comVal) {
        ++numModified;
      }
    }
    return numModified;
  }
};

TEST_F(RandomNeighborhoodTest, initialize) {
  initialize();
  expectHolds();

  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    _neighborhood->initialize(_random, *_assignment);
    expectHolds();
  }
}

TEST_F(RandomNeighborhoodTest, randomMove) {
  for (auto i = 0; i < 1000; i++) {
    initialize();
    expectHolds();
    const size_t actual = _neighborhood->randomMove(_random, *_assignment);
    const size_t expected = expectHolds();
    EXPECT_EQ(actual, expected);
  }
}

TEST_F(RandomNeighborhoodTest, commitIf) {
  initialize();
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    commitIf();
    expectHolds();
  }
}

}  // namespace atlantis::testing
