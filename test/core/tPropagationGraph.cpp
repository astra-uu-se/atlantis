#include <iostream>
#include <limits>
#include <random>
#include <vector>

#include "core/engine.hpp"
#include "core/savedInt.hpp"
#include "core/types.hpp"
#include "gtest/gtest.h"
#include "invariants/linear.hpp"
#include "propagation/propagationGraph.hpp"

namespace {

class PropagationGraphTest : public ::testing::Test {
 protected:
  std::mt19937 gen;

  std::unique_ptr<PropagationGraph> pg;

  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
    pg = std::make_unique<PropagationGraph>();
  }
};

TEST_F(PropagationGraphTest, RegisterVariable) {
  pg->registerVar(VarId(1));
  pg->registerVar(VarId(2));
  pg->registerVar(VarId(3));
  pg->registerVar(VarId(4));
  pg->registerVar(VarId(5));
  pg->registerVar(VarId(6));
  EXPECT_EQ(pg->getNumVariables(), 6);
}

TEST_F(PropagationGraphTest, RegisterInvariant) {
  pg->registerInvariant(InvariantId(1));
  pg->registerInvariant(InvariantId(2));
  pg->registerInvariant(InvariantId(3));
  pg->registerInvariant(InvariantId(4));
  pg->registerInvariant(InvariantId(5));
  EXPECT_EQ(pg->getNumInvariants(), 5);
}

TEST_F(PropagationGraphTest, TopologicalSort) {
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

  Int ignore = 0;

  // var1 and var2 defines var3 via invariant 1
  pg->registerInvariantDependsOnVar(InvariantId(1), VarId(1), ignore, ignore);
  pg->registerInvariantDependsOnVar(InvariantId(1), VarId(2), ignore, ignore);
  pg->registerDefinedVariable(VarId(3), InvariantId(1));

  // var4 and var5 defines var6 and var7 via invariant 2
  pg->registerInvariantDependsOnVar(InvariantId(2), VarId(4), ignore, ignore);
  pg->registerInvariantDependsOnVar(InvariantId(2), VarId(5), ignore, ignore);
  pg->registerDefinedVariable(VarId(6), InvariantId(2));
  pg->registerDefinedVariable(VarId(7), InvariantId(2));

  // Var3, var4, var6, var9 defines var8 via invariant 3
  pg->registerInvariantDependsOnVar(InvariantId(3), VarId(3), ignore, ignore);
  pg->registerInvariantDependsOnVar(InvariantId(3), VarId(4), ignore, ignore);
  pg->registerInvariantDependsOnVar(InvariantId(3), VarId(6), ignore, ignore);
  pg->registerInvariantDependsOnVar(InvariantId(3), VarId(9), ignore, ignore);
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

}  // namespace