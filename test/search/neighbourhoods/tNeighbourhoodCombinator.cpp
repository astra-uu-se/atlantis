#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "search/annealing/annealerContainer.hpp"
#include "search/neighbourhoods/neighbourhoodCombinator.hpp"

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

  MOCK_METHOD(const std::vector<search::SearchVariable>&, coveredVariables, (),
              (const override));
};

class NeighbourhoodCombinatorTest : public ::testing::Test {
 public:
  std::vector<std::shared_ptr<search::neighbourhoods::Neighbourhood>> ns;
  MockNeighbourhood* n1{nullptr};
  MockNeighbourhood* n2{nullptr};

  std::vector<search::SearchVariable> variables{
      search::SearchVariable(propagation::NULL_ID, SearchDomain(0, 10))};

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
  EXPECT_CALL(*n1, coveredVariables()).WillRepeatedly(ReturnRef(variables));
  EXPECT_CALL(*n2, coveredVariables()).WillRepeatedly(ReturnRef(variables));

  search::neighbourhoods::NeighbourhoodCombinator combinator(std::move(ns));

  propagation::PropagationEngine engine;
  search::RandomProvider random(123456789);

  engine.beginMove();
  search::AssignmentModifier modifier(engine);

  EXPECT_CALL(*n1, initialise(Ref(random), Ref(modifier))).Times(1);
  EXPECT_CALL(*n2, initialise(Ref(random), Ref(modifier))).Times(1);

  combinator.initialise(random, modifier);
  engine.endMove();
}

TEST_F(NeighbourhoodCombinatorTest,
       randomMove_calls_one_neighbourhood_and_forwards_result) {
  EXPECT_CALL(*n1, coveredVariables()).WillRepeatedly(ReturnRef(variables));
  EXPECT_CALL(*n2, coveredVariables()).WillRepeatedly(ReturnRef(variables));

  search::neighbourhoods::NeighbourhoodCombinator combinator(std::move(ns));

  propagation::PropagationEngine engine;
  search::RandomProvider random(123456789);
  search::Assignment assignment(engine, propagation::NULL_ID,
                                propagation::NULL_ID,
                                propagation::ObjectiveDirection::NONE);

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