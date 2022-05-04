#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "search/annealing/annealerFacade.hpp"
#include "search/neighbourhoods/neighbourhoodCombinator.hpp"

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
  std::vector<std::unique_ptr<search::neighbourhoods::Neighbourhood>> ns;
  MockNeighbourhood* n1{nullptr};
  MockNeighbourhood* n2{nullptr};

  std::vector<search::SearchVariable> variables{search::SearchVariable(NULL_ID, SearchDomain(0, 10))};

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
  EXPECT_CALL(*n1, coveredVariables())
    .WillRepeatedly(::testing::ReturnRef(variables));
  EXPECT_CALL(*n2, coveredVariables())
      .WillRepeatedly(::testing::ReturnRef(variables));

  search::neighbourhoods::NeighbourhoodCombinator combinator(std::move(ns));

  PropagationEngine engine;
  search::RandomProvider random(123456789);

  engine.beginMove();
  search::AssignmentModifier modifier(engine);

  EXPECT_CALL(*n1, initialise(::testing::Ref(random), ::testing::Ref(modifier)))
      .Times(1);
  EXPECT_CALL(*n2, initialise(::testing::Ref(random), ::testing::Ref(modifier)))
      .Times(1);

  combinator.initialise(random, modifier);
  engine.endMove();
}

TEST_F(NeighbourhoodCombinatorTest,
       randomMove_calls_one_neighbourhood_and_forwards_result) {
  EXPECT_CALL(*n1, coveredVariables())
      .WillRepeatedly(::testing::ReturnRef(variables));
  EXPECT_CALL(*n2, coveredVariables())
      .WillRepeatedly(::testing::ReturnRef(variables));

  search::neighbourhoods::NeighbourhoodCombinator combinator(std::move(ns));

  PropagationEngine engine;
  search::RandomProvider random(123456789);
  search::Assignment assignment(engine, NULL_ID, NULL_ID,
                                ObjectiveDirection::NONE);

  auto schedule = search::AnnealerFacade::cooling(0.95, 0.001, 50);
  schedule->start(10.0);
  search::Annealer annealer(assignment, random, *schedule);

  EXPECT_CALL(*n1,
              randomMove(::testing::Ref(random), ::testing::Ref(assignment),
                         ::testing::Ref(annealer)))
      .Times(0);

  EXPECT_CALL(*n2,
              randomMove(::testing::Ref(random), ::testing::Ref(assignment),
                         ::testing::Ref(annealer)))
      .WillOnce(::testing::Return(false));

  combinator.randomMove(random, assignment, annealer);
}
