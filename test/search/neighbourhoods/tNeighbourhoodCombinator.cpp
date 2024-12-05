#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../testHelper.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/search/annealing/annealerContainer.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/neighbourhoods/neighbourhoodCombinator.hpp"

namespace atlantis::testing {

using namespace atlantis::search::neighbourhoods;

using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;

class NeighbourhoodCombinatorTest : public ::testing::Test {
 public:
  std::shared_ptr<propagation::Solver> _solver;
  std::shared_ptr<Assignment> _assignment;
  std::shared_ptr<MockNeighbourhood> n1;
  std::shared_ptr<MockNeighbourhood> n2;
  std::shared_ptr<NeighbourhoodCombinator> _combinator;
  RandomProvider _random{123456789};

  std::vector<SearchVar> vars{
      SearchVar(propagation::NULL_ID, SearchDomain(0, 10))};

  void SetUp() override {
    n1 = std::make_shared<MockNeighbourhood>();
    EXPECT_CALL(*n1, coveredVars()).WillRepeatedly(ReturnRef(vars));

    n2 = std::make_shared<MockNeighbourhood>();
    EXPECT_CALL(*n2, coveredVars()).WillRepeatedly(ReturnRef(vars));

    auto ns = std::vector<std::shared_ptr<Neighbourhood>>{n1, n2};

    _combinator = std::make_shared<NeighbourhoodCombinator>(std::move(ns));

    _solver = std::make_shared<propagation::Solver>();

    _assignment = std::make_shared<Assignment>(
        *_solver, *_combinator, propagation::NULL_ID, propagation::NULL_ID,
        ObjectiveDirection::NONE, Int{0});
  }
};

TEST_F(NeighbourhoodCombinatorTest, initialise_calls_all_neighbourhoods) {
  EXPECT_CALL(*n1, initialise(Ref(_random), Ref(*_assignment))).Times(1);
  EXPECT_CALL(*n2, initialise(Ref(_random), Ref(*_assignment))).Times(1);

  _assignment->initialise(_random);
}

TEST_F(NeighbourhoodCombinatorTest,
       randomMove_calls_one_neighbourhood_and_forwards_result) {
  auto schedule = AnnealerContainer::cooling(0.95, 4);
  Annealer annealer(_random, *schedule, *_assignment);
  annealer.start();

  EXPECT_CALL(*n1, randomMove(Ref(_random), Ref(*_assignment))).Times(0);

  EXPECT_CALL(*n2, randomMove(Ref(_random), Ref(*_assignment)))
      .WillOnce(Return(false));

  _assignment->performProbe(_random);
}

}  // namespace atlantis::testing
