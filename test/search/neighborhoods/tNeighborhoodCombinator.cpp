#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../testHelper.hpp"
#include "./testHelper.hpp"
#include "atlantis/search/neighborhoods/neighborhoodCombinator.hpp"

namespace atlantis::testing {

using namespace atlantis::search::neighborhoods;

using ::testing::AtMost;
using ::testing::Exactly;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;

class NeighborhoodCombinatorTest : public NeighborhoodTestBase {
 public:
  std::shared_ptr<MockNeighborhood> n1;
  std::shared_ptr<MockNeighborhood> n2;
  std::shared_ptr<NeighborhoodCombinator> _combinator;

  std::vector<SearchVar> vars;

  void SetUp() override {
    NeighborhoodTestBase::SetUp();

    vars = std::vector<SearchVar>{
        SearchVar(propagation::NULL_ID, SearchDomain(0, 10))};

    n1 = std::make_shared<MockNeighborhood>();
    EXPECT_CALL(*n1, coveredVars()).WillRepeatedly(ReturnRef(vars));

    n2 = std::make_shared<MockNeighborhood>();
    EXPECT_CALL(*n2, coveredVars()).WillRepeatedly(ReturnRef(vars));

    _combinator = std::make_shared<NeighborhoodCombinator>(
        std::vector<std::shared_ptr<Neighborhood>>{n1, n2});
  }
};

TEST_F(NeighborhoodCombinatorTest, initialize) {
  EXPECT_CALL(*n1, initialize(Ref(_random), Ref(*_assignment))).Times(1);
  EXPECT_CALL(*n2, initialize(Ref(_random), Ref(*_assignment))).Times(1);

  _combinator->initialize(_random, *_assignment);
}

TEST_F(NeighborhoodCombinatorTest, randomMove) {
  EXPECT_CALL(*n2, randomMove(Ref(_random), Ref(*_assignment)))
      .Times(AtMost(1))
      .WillOnce(Return(size_t{0}));

  EXPECT_CALL(*n2, randomMove(Ref(_random), Ref(*_assignment)))
      .Times(AtMost(1))
      .WillOnce(Return(size_t{1}));

  _combinator->randomMove(_random, *_assignment);
}

TEST_F(NeighborhoodCombinatorTest, commitIf) {
  EXPECT_CALL(*n2, randomMove(Ref(_random), Ref(*_assignment)))
      .Times(AtMost(1))
      .WillOnce(Return(size_t{0}));

  EXPECT_CALL(*n2, randomMove(Ref(_random), Ref(*_assignment)))
      .Times(AtMost(1))
      .WillOnce(Return(size_t{1}));

  size_t nIndex = _combinator->randomMove(_random, *_assignment);

  EXPECT_CALL(*n2, commitIf(Ref(*_assignment)))
      .Times(Exactly(nIndex == 0 ? 1 : 0));

  EXPECT_CALL(*n2, commitIf(Ref(*_assignment)))
      .Times(Exactly(nIndex == 0 ? 0 : 1));

  _combinator->commitIf(*_assignment);
}

}  // namespace atlantis::testing
