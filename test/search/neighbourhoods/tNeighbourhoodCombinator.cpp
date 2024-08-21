#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "atlantis/search/annealing/annealerContainer.hpp"
#include "atlantis/search/neighbourhoods/neighbourhoodCombinator.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;

class MockNeighbourhood : public search::neighbourhoods::Neighbourhood {
 public:
  MOCK_METHOD(void, initialise,
              (search::RandomProvider&, search::AssignmentModifier&),
              (override));

  MOCK_METHOD(bool, randomMove,
              (search::RandomProvider&, search::Assignment&, search::Annealer&),
              (override));

  MOCK_METHOD(const std::vector<search::SearchVar>&, coveredVars, (),
              (const override));
};

class NeighbourhoodCombinatorTest : public ::testing::Test {
 public:
  std::vector<std::shared_ptr<search::neighbourhoods::Neighbourhood>> ns;
  MockNeighbourhood* n1{nullptr};
  MockNeighbourhood* n2{nullptr};

  std::vector<search::SearchVar> vars{
      search::SearchVar(propagation::NULL_ID, SearchDomain(0, 10))};

  void SetUp() override {
    auto unique_n1 = std::make_unique<MockNeighbourhood>();
    auto unique_n2 = std::make_unique<MockNeighbourhood>();

    n1 = unique_n1.get();
    n2 = unique_n2.get();

    ns.push_back(std::move(unique_n1));
    ns.push_back(std::move(unique_n2));
  }
};

TEST_F(NeighbourhoodCombinatorTest, initialise_calls_all_neighbourhoods) {
  EXPECT_CALL(*n1, coveredVars()).WillRepeatedly(ReturnRef(vars));
  EXPECT_CALL(*n2, coveredVars()).WillRepeatedly(ReturnRef(vars));

  search::neighbourhoods::NeighbourhoodCombinator combinator(std::move(ns));

  propagation::Solver solver;
  search::RandomProvider random(123456789);

  solver.beginMove();
  search::AssignmentModifier modifier(solver);

  EXPECT_CALL(*n1, initialise(Ref(random), Ref(modifier))).Times(1);
  EXPECT_CALL(*n2, initialise(Ref(random), Ref(modifier))).Times(1);

  combinator.initialise(random, modifier);
  solver.endMove();
}

TEST_F(NeighbourhoodCombinatorTest,
       randomMove_calls_one_neighbourhood_and_forwards_result) {
  EXPECT_CALL(*n1, coveredVars()).WillRepeatedly(ReturnRef(vars));
  EXPECT_CALL(*n2, coveredVars()).WillRepeatedly(ReturnRef(vars));

  search::neighbourhoods::NeighbourhoodCombinator combinator(std::move(ns));

  propagation::Solver solver;
  search::RandomProvider random(123456789);
  search::Assignment assignment(solver, propagation::NULL_ID,
                                propagation::NULL_ID,
                                propagation::ObjectiveDirection::NONE, Int{0});

  auto schedule = search::AnnealerContainer::cooling(0.95, 4);
  search::Annealer annealer(assignment, random, *schedule);
  annealer.start();

  EXPECT_CALL(*n1, randomMove(Ref(random), Ref(assignment), Ref(annealer)))
      .Times(0);

  EXPECT_CALL(*n2, randomMove(Ref(random), Ref(assignment), Ref(annealer)))
      .WillOnce(Return(false));

  combinator.randomMove(random, assignment, annealer);
}

}  // namespace atlantis::testing
