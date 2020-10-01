#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <vector>

#include "core/propagationEngine.hpp"
#include "core/savedInt.hpp"
#include "core/types.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "views/intMaxView.hpp"
#include "invariants/linear.hpp"

namespace {

class IntMaxViewTest : public ::testing::Test {
 protected:
  std::mt19937 gen;

  std::unique_ptr<PropagationEngine> engine;

  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
    engine = std::make_unique<PropagationEngine>();
  }
};

TEST_F(IntMaxViewTest, CreateIntMaxView) {
  engine->open();

  const VarId var = engine->makeIntVar(100, 0, 100);
  std::shared_ptr<IntMaxView> viewOfVar = engine->makeIntVarView<IntMaxView>(var, 50);
  std::shared_ptr<IntMaxView> viewOfView = engine->makeIntVarView<IntMaxView>(viewOfVar->getId(), 25);

  EXPECT_EQ(viewOfView->getCommittedValue(), Int(25));
  EXPECT_EQ(viewOfVar->getCommittedValue(), Int(50));

  engine->close();
}

TEST_F(IntMaxViewTest, RecomputeIntMaxView) {
  engine->open();
  auto a = engine->makeIntVar(20, -100, 100);
  auto b = engine->makeIntVar(20, -100, 100);
  auto sum = engine->makeIntVar(0, -100, 100);

  auto linear = engine->makeInvariant<Linear>(std::vector<Int>({1, 1}),
                                              std::vector<VarId>({a, b}), sum);
  
  std::shared_ptr<IntMaxView> viewOfVar = engine->makeIntVarView<IntMaxView>(sum, 10);
  std::shared_ptr<IntMaxView> viewOfView = engine->makeIntVarView<IntMaxView>(viewOfVar->getId(), 5);
  VarId viewOfVarId = viewOfVar->getId();
  VarId viewOfViewId = viewOfView->getId();

  EXPECT_EQ(engine->getValue(viewOfVarId), Int(0));
  EXPECT_EQ(engine->getValue(viewOfViewId), Int(0));
  
  engine->close();

  EXPECT_EQ(engine->getValue(sum), Int(40));
  EXPECT_EQ(engine->getValue(viewOfVarId), Int(10));
  EXPECT_EQ(engine->getValue(viewOfViewId), Int(5));

  engine->beginMove();
  engine->updateValue(a, 1);
  engine->updateValue(b, 1);
  engine->endMove();

  engine->beginQuery();
  engine->query(sum);
  engine->endQuery();


  EXPECT_EQ(engine->getValue(sum), Int(2));
  EXPECT_EQ(engine->getValue(viewOfViewId), Int(2));
  EXPECT_EQ(engine->getValue(viewOfVarId), Int(2));

}

TEST_F(IntMaxViewTest, PropagateIntVarViews) {
  engine->open();
  auto a = engine->makeIntVar(20, -100, 100); // VarId 1
  auto b = engine->makeIntVar(20, -100, 100); // VarId 2
  auto sum1 = engine->makeIntVar(0, -100, 100); // VarId 3
  // a + b = sum1
  auto c = engine->makeIntVar(20, -100, 100); // VarId 4
  auto d = engine->makeIntVar(20, -100, 100); // VarId 5
  auto sum2 = engine->makeIntVar(0, -100, 100); // VarId 6
  // c + d = sum2
  auto sum3 = engine->makeIntVar(0, -100, 100); // VarId 7
  // sum1 + sum2 = sum2

  // InvariantId 1 depends on VarIds {a, b} = {1, 2}
  auto linear1 = engine->makeInvariant<Linear>(std::vector<Int>({1, 1}),
                                               std::vector<VarId>({a, b}), sum1);
  // InvariantId 2 depends on VarIds {c, d} = {3, 4}
  auto linear2 = engine->makeInvariant<Linear>(std::vector<Int>({1, 1}),
                                               std::vector<VarId>({c, d}), sum2);
  
  // IntMaxView (VarId::view 1) depends on VarId sum1=3
  std::shared_ptr<IntMaxView> sum1View = engine->makeIntVarView<IntMaxView>(sum1, 20);
  // IntMaxView (VarId::view 2) depends on VarId sum2=6
  std::shared_ptr<IntMaxView> sum2View = engine->makeIntVarView<IntMaxView>(sum2, 20);

  // IntMaxView (VarId::view 1)
  VarId sum1ViewId = sum1View->getId();
  // IntMaxView (VarId::view 2)
  VarId sum2ViewId = sum2View->getId();

  // InvariantId 3 depends on VarId::views {sum1ViewId, sum2ViewId} = {1, 2}
  // that in turn depend on {sum1, sum2} = {3, 6}
  auto linear3 = engine->makeInvariant<Linear>(std::vector<Int>({1, 1}),
                                               std::vector<VarId>({sum1ViewId, sum2ViewId}), sum3);

  std::vector<VarId> sum3viewIds;
  std::vector<std::shared_ptr<IntMaxView>> sum3views;
  sum3viewIds.reserve(10);
  VarId prev = sum3;
  for (size_t i = 0; i < 10; ++i) {
    // IntMaxView (VarId::view 3+i) depends on
    //   * VarId sum3 if i = 0,
    //   * VarId::view 2+i otherwise
    sum3views.emplace_back(engine->makeIntVarView<IntMaxView>(prev, 25 - i));
    sum3viewIds.emplace_back(sum3views[i]->getId());
    // IntMaxView (VarId::view 3+i)
    prev = sum3viewIds[i];
  }
  
  EXPECT_EQ(engine->getCommittedValue(sum1), Int(0));
  EXPECT_EQ(engine->getCommittedValue(sum2), Int(0));
  EXPECT_EQ(engine->getCommittedValue(sum1ViewId), Int(0));
  EXPECT_EQ(engine->getCommittedValue(sum2ViewId), Int(0));

  engine->close();
  
  // a + b = 20 + 20 = sum1 = 40
  EXPECT_EQ(engine->getValue(sum1), Int(40));
  // sum1 = 40 -> sum1View = min(20, 40) = 20
  EXPECT_EQ(engine->getValue(sum1ViewId), Int(20));
  // c + d = 20 + 20 = sum2 = 40
  EXPECT_EQ(engine->getValue(sum2), Int(40));
  // sum2 = 40 -> sum2View = min(20, 40) = 20
  EXPECT_EQ(engine->getValue(sum2ViewId), Int(20));
  // sum3 = sum1view + sum2view = 20 + 20 = 40
  EXPECT_EQ(engine->getValue(sum3), Int(40));
  
  for (size_t i = 0; i < 10; ++i) {
    // IntMaxView sum3viewIds[i] = 
    //   * min(40, 25) if i = 0,
    //   * min(25-i, sum3viewIds[i-1]) otherwise
    EXPECT_EQ(engine->getCommittedValue(sum3viewIds[i]), Int(25 - i));
  }


  engine->beginMove();
  engine->updateValue(a, 5);
  engine->updateValue(b, 5);
  engine->updateValue(c, 5);
  engine->updateValue(d, 5);
  engine->endMove();

  EXPECT_EQ(engine->getValue(a), Int(5));
  EXPECT_EQ(engine->getValue(b), Int(5));
  EXPECT_EQ(engine->getValue(c), Int(5));
  EXPECT_EQ(engine->getValue(d), Int(5));

  engine->beginCommit();
  engine->query(sum1);
  engine->query(sum2);
  engine->query(sum3);
  engine->endCommit();

  // a + b = 5 + 5 = sum1 = 10
  EXPECT_EQ(engine->getValue(sum1), Int(10));
  // c + d = 5 + 5 = sum2 = 10
  EXPECT_EQ(engine->getValue(sum2), Int(10));

  // sum1 = 10 -> sum1View = min(20, 10) = 10
  EXPECT_EQ(engine->getValue(sum1ViewId), Int(10));
  // sum2 = 10 -> sum2View = min(20, 10) = 10
  EXPECT_EQ(engine->getValue(sum2ViewId), Int(10));
  

  // sum3 = sum1view + sum2view = 10 + 19 = 20
  // sum1viewId = VarId::view 1
  // sum1viewId = VarId::view 2

  // It looks as if the third invariant calculating sum3
  // is neither committed after endMove nor endCommit.
  // The value for sum1View and sum2view have changed,
  // but not sum3.
  /* Sketch of invariant graph:
     Ids are in braces {}

   a {1}      b {2}   c {3}      d {4}
   |          |       |          |
 __V__________V_    __V__________V_
|    Linear     |  |    Linear     |
|      {1}      |  |      {2}      |
|_______________|  |_______________|
            |          |
            V          V
          sum1 {5}   sum2 {6}
            |          |
            V          V
     sum1view {1}   sum2view {2}
            |          |
          __V__________V_
         |    Linear     |
         |      {3}      |
         |_______________|
                 | 
                 V
               sum3 {7}   <----- This does not get calculated
                 |
                 V
           sum3viewIds[0] {3}
                 |
                 V
                ...
                 |
                 V
           sum3viewIds[9] {12}
  */
  // These two lines should not be needed!
  
  EXPECT_EQ(engine->getValue(sum3), Int(20));
  
  for (size_t i = 0; i < sum3viewIds.size(); ++i) {
    // IntMaxView sum3viewIds[i] = 
    //   * min(20, 25) if i = 0,
    //   * min(25-i, sum3viewIds[i-1]) otherwise
    // should be 25 for the first 6, then
    // [19, 18, 17, 16]
    EXPECT_EQ(engine->getValue(sum3viewIds[i]), std::min(Int(25 - i), Int(20)));
  }

}

}