#include <iostream>
#include <limits>
#include <random>
#include <vector>

#include "core/engine.hpp"
#include "core/savedInt.hpp"
#include "core/types.hpp"
#include "gtest/gtest.h"
#include "invariants/linear.hpp"
#include "propagation/topDownPropagationGraph.hpp"

namespace {

class TopDownPropagationGraphTest : public ::testing::Test {
 protected:
  std::mt19937 gen;

  std::unique_ptr<TopDownPropagationGraph> pg;

  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
    pg = std::make_unique<TopDownPropagationGraph>();
  }
};

TEST_F(TopDownPropagationGraphTest, RegisterVariable) {
  pg->registerVar(VarId(1));
  pg->registerVar(VarId(2));
  pg->registerVar(VarId(3));
  pg->registerVar(VarId(4));
  pg->registerVar(VarId(5));
  pg->registerVar(VarId(6));
  EXPECT_EQ(pg->getNumVariables(), size_t(6));
}

TEST_F(TopDownPropagationGraphTest, RegisterInvariant) {
  pg->registerInvariant(InvariantId(1));
  pg->registerInvariant(InvariantId(2));
  pg->registerInvariant(InvariantId(3));
  pg->registerInvariant(InvariantId(4));
  pg->registerInvariant(InvariantId(5));
  EXPECT_EQ(pg->getNumInvariants(), size_t(5));
}

TEST_F(TopDownPropagationGraphTest, TopologicalSort) {
  pg->registerVar(VarId(1));
  pg->registerVar(VarId(2));
  pg->registerVar(VarId(3));
  pg->registerVar(VarId(4));
  pg->registerVar(VarId(5));
  pg->registerVar(VarId(6));
  pg->registerVar(VarId(7));
  pg->registerVar(VarId(8));
  pg->registerVar(VarId(9));
  pg->registerInvariant(InvariantId(1));
  pg->registerInvariant(InvariantId(2));
  pg->registerInvariant(InvariantId(3));

  // var1 and var2 defines var3 via invariant 1
  pg->registerInvariantDependsOnVar(InvariantId(1), VarId(1));
  pg->registerInvariantDependsOnVar(InvariantId(1), VarId(2));
  pg->registerDefinedVariable(VarId(3), InvariantId(1));

  // var4 and var5 defines var6 and var7 via invariant 2
  pg->registerInvariantDependsOnVar(InvariantId(2), VarId(4));
  pg->registerInvariantDependsOnVar(InvariantId(2), VarId(5));
  pg->registerDefinedVariable(VarId(6), InvariantId(2));
  pg->registerDefinedVariable(VarId(7), InvariantId(2));

  // Var3, var4, var6, var9 defines var8 via invariant 3
  pg->registerInvariantDependsOnVar(InvariantId(3), VarId(3));
  pg->registerInvariantDependsOnVar(InvariantId(3), VarId(4));
  pg->registerInvariantDependsOnVar(InvariantId(3), VarId(6));
  pg->registerInvariantDependsOnVar(InvariantId(3), VarId(9));
  pg->registerDefinedVariable(VarId(8), InvariantId(3));

  // Var 1,2,4,5,9 are top level variables.
  // var 7 and 8 are bottom variables.

  EXPECT_NO_THROW(pg->close());

  // all we know about they keys is that the respect the topological order. But
  // keys "at the same level" are not necessarily equal.
  EXPECT_LT(pg->getTopologicalKey(VarId(1)), pg->getTopologicalKey(VarId(3)));
  EXPECT_LT(pg->getTopologicalKey(VarId(2)), pg->getTopologicalKey(VarId(3)));

  EXPECT_LT(pg->getTopologicalKey(VarId(4)), pg->getTopologicalKey(VarId(6)));
  EXPECT_LT(pg->getTopologicalKey(VarId(5)), pg->getTopologicalKey(VarId(6)));
  EXPECT_LT(pg->getTopologicalKey(VarId(4)), pg->getTopologicalKey(VarId(7)));
  EXPECT_LT(pg->getTopologicalKey(VarId(5)), pg->getTopologicalKey(VarId(7)));

  EXPECT_LT(pg->getTopologicalKey(VarId(3)), pg->getTopologicalKey(VarId(8)));
  EXPECT_LT(pg->getTopologicalKey(VarId(6)), pg->getTopologicalKey(VarId(8)));
  EXPECT_LT(pg->getTopologicalKey(VarId(9)), pg->getTopologicalKey(VarId(8)));

  EXPECT_LT(pg->getTopologicalKey(InvariantId(1)),
            pg->getTopologicalKey(InvariantId(3)));

  EXPECT_LT(pg->getTopologicalKey(InvariantId(2)),
            pg->getTopologicalKey(InvariantId(3)));
}

TEST_F(TopDownPropagationGraphTest, TopologicalSortLongChain) {
  pg->registerVar(VarId(1));
  pg->registerVar(VarId(2));
  pg->registerVar(VarId(3));
  pg->registerVar(VarId(4));
  pg->registerVar(VarId(5));
  pg->registerVar(VarId(6));
  pg->registerVar(VarId(7));
  pg->registerVar(VarId(8));
  pg->registerInvariant(InvariantId(1));
  pg->registerInvariant(InvariantId(2));
  pg->registerInvariant(InvariantId(3));
  pg->registerInvariant(InvariantId(4));
  pg->registerInvariant(InvariantId(5));
  pg->registerInvariant(InvariantId(6));
  pg->registerInvariant(InvariantId(7));

  pg->registerInvariantDependsOnVar(InvariantId(6), VarId(6));
  pg->registerInvariantDependsOnVar(InvariantId(6), VarId(1));
  pg->registerDefinedVariable(VarId(7), InvariantId(6));

  pg->registerInvariantDependsOnVar(InvariantId(1), VarId(1));
  pg->registerDefinedVariable(VarId(2), InvariantId(1));

  pg->registerInvariantDependsOnVar(InvariantId(2), VarId(2));
  pg->registerDefinedVariable(VarId(3), InvariantId(2));

  pg->registerInvariantDependsOnVar(InvariantId(3), VarId(3));
  pg->registerDefinedVariable(VarId(4), InvariantId(3));

  pg->registerInvariantDependsOnVar(InvariantId(4), VarId(4));
  pg->registerDefinedVariable(VarId(5), InvariantId(4));

  pg->registerInvariantDependsOnVar(InvariantId(5), VarId(5));
  pg->registerDefinedVariable(VarId(6), InvariantId(5));

  pg->registerInvariantDependsOnVar(InvariantId(7), VarId(7));
  pg->registerDefinedVariable(VarId(8), InvariantId(7));

  EXPECT_NO_THROW(pg->close());

  EXPECT_LT(pg->getTopologicalKey(VarId(1)), pg->getTopologicalKey(VarId(5)));
  EXPECT_LT(pg->getTopologicalKey(VarId(1)), pg->getTopologicalKey(VarId(6)));
  EXPECT_LT(pg->getTopologicalKey(VarId(1)), pg->getTopologicalKey(VarId(7)));
  EXPECT_LT(pg->getTopologicalKey(VarId(1)), pg->getTopologicalKey(VarId(8)));

  EXPECT_LT(pg->getTopologicalKey(VarId(5)), pg->getTopologicalKey(VarId(6)));
  EXPECT_LT(pg->getTopologicalKey(VarId(5)), pg->getTopologicalKey(VarId(7)));
  EXPECT_LT(pg->getTopologicalKey(VarId(5)), pg->getTopologicalKey(VarId(8)));

  EXPECT_LT(pg->getTopologicalKey(InvariantId(1)),
            pg->getTopologicalKey(InvariantId(6)));

  EXPECT_LT(pg->getTopologicalKey(InvariantId(5)),
            pg->getTopologicalKey(InvariantId(6)));

  EXPECT_LT(pg->getTopologicalKey(InvariantId(1)),
            pg->getTopologicalKey(InvariantId(7)));

  EXPECT_LT(pg->getTopologicalKey(InvariantId(5)),
            pg->getTopologicalKey(InvariantId(7)));
}

TEST_F(TopDownPropagationGraphTest, TopologicalSortSimpleChainCycles) {
  pg->registerVar(VarId(1));
  pg->registerVar(VarId(2));
  pg->registerVar(VarId(3));
  pg->registerVar(VarId(4));
  pg->registerVar(VarId(5));
  pg->registerVar(VarId(6));
  pg->registerVar(VarId(7));
  pg->registerInvariant(InvariantId(1));
  pg->registerInvariant(InvariantId(2));
  pg->registerInvariant(InvariantId(3));
  pg->registerInvariant(InvariantId(4));
  pg->registerInvariant(InvariantId(5));
  pg->registerInvariant(InvariantId(6));

  pg->registerInvariantDependsOnVar(InvariantId(1), VarId(1));
  pg->registerDefinedVariable(VarId(2), InvariantId(1));
  // Cycle 1
  pg->registerInvariantDependsOnVar(InvariantId(2), VarId(2));
  pg->registerDefinedVariable(VarId(3), InvariantId(2));
  pg->registerInvariantDependsOnVar(InvariantId(1), VarId(3));

  pg->registerInvariantDependsOnVar(InvariantId(3), VarId(3));
  pg->registerDefinedVariable(VarId(4), InvariantId(3));
  pg->registerInvariantDependsOnVar(InvariantId(4), VarId(4));
  pg->registerDefinedVariable(VarId(5), InvariantId(4));
  // Cycle2
  pg->registerInvariantDependsOnVar(InvariantId(5), VarId(5));
  pg->registerDefinedVariable(VarId(6), InvariantId(5));
  pg->registerInvariantDependsOnVar(InvariantId(4), VarId(6));

  pg->registerInvariantDependsOnVar(InvariantId(6), VarId(6));
  pg->registerDefinedVariable(VarId(7), InvariantId(6));

  EXPECT_NO_THROW(pg->close());

  EXPECT_LT(pg->getTopologicalKey(VarId(1)), pg->getTopologicalKey(VarId(2)));
  EXPECT_LT(pg->getTopologicalKey(VarId(1)), pg->getTopologicalKey(VarId(3)));
  EXPECT_LT(pg->getTopologicalKey(VarId(1)), pg->getTopologicalKey(VarId(4)));
  EXPECT_LT(pg->getTopologicalKey(VarId(2)), pg->getTopologicalKey(VarId(4)));
  EXPECT_LT(pg->getTopologicalKey(VarId(3)), pg->getTopologicalKey(VarId(4)));

  EXPECT_LT(pg->getTopologicalKey(VarId(4)), pg->getTopologicalKey(VarId(5)));
  EXPECT_LT(pg->getTopologicalKey(VarId(4)), pg->getTopologicalKey(VarId(6)));
  EXPECT_LT(pg->getTopologicalKey(VarId(4)), pg->getTopologicalKey(VarId(7)));
  EXPECT_LT(pg->getTopologicalKey(VarId(5)), pg->getTopologicalKey(VarId(7)));
  EXPECT_LT(pg->getTopologicalKey(VarId(6)), pg->getTopologicalKey(VarId(7)));
}

}  // namespace