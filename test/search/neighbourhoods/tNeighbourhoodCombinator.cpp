#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../testHelper.hpp"
#include "./testHelper.hpp"
#include "atlantis/search/neighbourhoods/neighbourhoodCombinator.hpp"

namespace atlantis::testing {

using namespace atlantis::search::neighbourhoods;

using ::testing::AtMost;
using ::testing::Exactly;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;

class NeighbourhoodCombinatorTest : public NeighbourhoodTestBase {
 public:
  std::shared_ptr<MockNeighbourhood> n1;
  std::shared_ptr<MockNeighbourhood> n2;
  std::shared_ptr<NeighbourhoodCombinator> _combinator;

  std::vector<SearchVar> vars;

  void SetUp() override {
    NeighbourhoodTestBase::SetUp();

    vars = std::vector<SearchVar>{
        SearchVar(propagation::NULL_ID, SearchDomain(0, 10))};

    n1 = std::make_shared<MockNeighbourhood>();
    EXPECT_CALL(*n1, coveredVars()).WillRepeatedly(ReturnRef(vars));

    n2 = std::make_shared<MockNeighbourhood>();
    EXPECT_CALL(*n2, coveredVars()).WillRepeatedly(ReturnRef(vars));

    _combinator = std::make_shared<NeighbourhoodCombinator>(
        std::vector<std::shared_ptr<Neighbourhood>>{n1, n2});
  }
};

TEST_F(NeighbourhoodCombinatorTest, initialize) {
  EXPECT_CALL(*n1, initialize(Ref(_random), Ref(*_assignment))).Times(1);
  EXPECT_CALL(*n2, initialize(Ref(_random), Ref(*_assignment))).Times(1);

  _combinator->initialize(_random, *_assignment);
}

TEST_F(NeighbourhoodCombinatorTest, randomMove) {
  EXPECT_CALL(*n2, randomMove(Ref(_random), Ref(*_assignment)))
      .Times(AtMost(1))
      .WillOnce(Return(size_t{0}));

  EXPECT_CALL(*n2, randomMove(Ref(_random), Ref(*_assignment)))
      .Times(AtMost(1))
      .WillOnce(Return(size_t{1}));

  _combinator->randomMove(_random, *_assignment);
}

TEST_F(NeighbourhoodCombinatorTest, commitIf) {
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
