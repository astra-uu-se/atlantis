#include <gtest/gtest.h>

#include <string>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/invariantGraphRoot.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElementNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/intLinearNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/intPlusNode.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

TEST(InvariantGraphTest, apply_result) {
  propagation::Solver solver;
  InvariantGraph invariantGraph(solver);

  const VarNodeId a =
      invariantGraph.retrieveIntVarNode(SearchDomain(0, 10), "a");
  const VarNodeId b =
      invariantGraph.retrieveIntVarNode(SearchDomain(0, 10), "b");
  const VarNodeId output =
      invariantGraph.retrieveIntVarNode(SearchDomain(0, 10), "output");

  EXPECT_TRUE(invariantGraph.containsVarNode("a"));
  EXPECT_TRUE(invariantGraph.containsVarNode("b"));
  EXPECT_TRUE(invariantGraph.containsVarNode("output"));

  invariantGraph.addInvariantNode(
      std::make_shared<IntPlusNode>(invariantGraph, a, b, output));

  invariantGraph.addImplicitConstraintNode(std::make_shared<InvariantGraphRoot>(
      invariantGraph, std::vector<VarNodeId>{a, b}));

  invariantGraph.apply();
  invariantGraph.close();

  EXPECT_NE(invariantGraph.varNode(a).varId(), propagation::NULL_ID);
  EXPECT_NE(invariantGraph.varNode(b).varId(), propagation::NULL_ID);
  EXPECT_NE(invariantGraph.varNode(output).varId(), propagation::NULL_ID);
}

TEST(InvariantGraphTest, ApplyGraph) {
  propagation::Solver solver;
  InvariantGraph invariantGraph(solver);

  const VarNodeId a1 = invariantGraph.retrieveIntVarNode(SearchDomain(0, 10));
  const VarNodeId a2 = invariantGraph.retrieveIntVarNode(SearchDomain(0, 10));
  const VarNodeId b1 = invariantGraph.retrieveIntVarNode(SearchDomain(0, 10));
  const VarNodeId b2 = invariantGraph.retrieveIntVarNode(SearchDomain(0, 10));

  const VarNodeId output1 =
      invariantGraph.retrieveIntVarNode(SearchDomain(0, 20));
  const VarNodeId output2 =
      invariantGraph.retrieveIntVarNode(SearchDomain(0, 20));
  const VarNodeId output3 =
      invariantGraph.retrieveIntVarNode(SearchDomain(0, 40));

  invariantGraph.addInvariantNode(
      std::make_shared<IntPlusNode>(invariantGraph, a1, a2, output1));
  invariantGraph.addInvariantNode(
      std::make_shared<IntPlusNode>(invariantGraph, b1, b2, output2));
  invariantGraph.addInvariantNode(
      std::make_shared<IntPlusNode>(invariantGraph, output1, output2, output3));

  invariantGraph.apply();
  invariantGraph.close();

  // 7 variables
  EXPECT_GE(solver.numVars(), 7);
  // dummy objective + dummy violation
  EXPECT_LE(solver.numVars(), 7 + 2);
  EXPECT_EQ(solver.numInvariants(), 3);
}

TEST(InvariantGraphTest, SplitSimpleGraph) {
  propagation::Solver solver;
  InvariantGraph invariantGraph(solver);
  /* Graph:
   *
   *  a   b     c   d
   *  |   |     |   |
   *  v   v     v   v
   * +-----+   +-----+
   * | pl1 |   | pl2 |
   * +-----+   +-----+
   *    |         |
   *    +----+----+
   *         |
   *         v
   *       output
   *
   */

  const VarNodeId a = invariantGraph.retrieveIntVarNode(SearchDomain(0, 10));
  const VarNodeId b = invariantGraph.retrieveIntVarNode(SearchDomain(0, 10));
  const VarNodeId c = invariantGraph.retrieveIntVarNode(SearchDomain(0, 10));
  const VarNodeId d = invariantGraph.retrieveIntVarNode(SearchDomain(0, 10));
  const VarNodeId output =
      invariantGraph.retrieveIntVarNode(SearchDomain(0, 20));

  invariantGraph.addInvariantNode(
      std::make_shared<IntPlusNode>(invariantGraph, a, b, output));

  invariantGraph.addInvariantNode(
      std::make_shared<IntPlusNode>(invariantGraph, c, d, output));

  invariantGraph.apply();
  invariantGraph.close();

  // a, b, c, d, output
  // x_copy
  // violation for equal constraint (x == x_copy)
  // dummy objective
  EXPECT_EQ(solver.numVars(), 5 + 1 + 1 + 1);
  // 2 Linear
  // 1 equal
  EXPECT_EQ(solver.numInvariants(), 3);
}

TEST(InvariantGraphTest, SplitGraph) {
  propagation::Solver solver;
  InvariantGraph invariantGraph(solver);
  /* Graph:
   *
   * 0_0 0_1   0_0 0_1      n_0 n_1
   *  |   |     |   |        |   |
   *  v   v     v   v        v   v
   * +-----+   +-----+      +-----+
   * | pl1 |   | pl2 |      | pln |
   * +-----+   +-----+      +-----+
   *    |         |             |
   *    +----+----+---- ... ----+
   *         |
   *         v
   *       output
   *
   */

  const size_t numInvariants = 5;
  const size_t numInputs = 5;
  const Int lb = 0;
  const Int ub = 10;
  const VarNodeId output = invariantGraph.retrieveIntVarNode(
      SearchDomain(lb * numInputs, ub * numInputs));

  std::vector<std::vector<VarNodeId>> varNodeIdMatrix(numInvariants,
                                                      std::vector<VarNodeId>{});
  for (size_t i = 0; i < numInvariants; ++i) {
    for (size_t j = 0; j < numInputs; ++j) {
      const std::string identifier(std::to_string(i) + "_" + std::to_string(j));
      varNodeIdMatrix.at(i).emplace_back(invariantGraph.retrieveIntVarNode(
          SearchDomain(lb, ub), std::string(identifier)));
    }
  }

  std::vector<Int> coeffs(numInputs, 1);
  for (const auto& identifierArray : varNodeIdMatrix) {
    std::vector<VarNodeId> inputVarNodeIds(identifierArray);
    invariantGraph.addInvariantNode(std::make_shared<IntLinearNode>(
        invariantGraph, std::vector<Int>{coeffs}, std::move(inputVarNodeIds),
        output));
  }

  invariantGraph.apply();
  invariantGraph.close();

  // Each invariant has numInputs inputs
  // Each invariant has 1 output
  // There is one violation for the AllDiff
  // One view for the AllDiff
  EXPECT_EQ(solver.numVars(), numInvariants * (numInputs + 1) + 2);
  EXPECT_EQ(solver.numInvariants(), numInvariants + 1);
}

TEST(InvariantGraphTest, BreakSimpleCycle) {
  propagation::Solver solver;
  InvariantGraph invariantGraph(solver);
  /* Graph:
   *
   *      +---------------+
   *      |               |
   * x1   |   +---+   x2  |
   *  |   |   |   |   |   |
   *  v   v   |   v   v   |
   * +-----+  |  +-----+  |
   * | pl1 |  |  | pl2 |  |
   * +-----+  |  +-----+  |
   *    |     |     |     |
   *    v     |     v     |
   * output1--+  output2--+
   *
   */

  const VarNodeId x1 = invariantGraph.retrieveIntVarNode(SearchDomain(0, 10));
  const VarNodeId x2 = invariantGraph.retrieveIntVarNode(SearchDomain(0, 10));
  const VarNodeId output1 = invariantGraph.retrieveIntVarNode(
      SearchDomain(0, 40), VarNode::DomainType::NONE);
  const VarNodeId output2 = invariantGraph.retrieveIntVarNode(
      SearchDomain(0, 40), VarNode::DomainType::NONE);

  invariantGraph.addInvariantNode(
      std::make_shared<IntPlusNode>(invariantGraph, x1, output2, output1));

  invariantGraph.addInvariantNode(
      std::make_shared<IntPlusNode>(invariantGraph, output1, x2, output2));

  invariantGraph.apply();
  invariantGraph.close();

  // x1, x2, output1, output2
  // the pivot
  // The Equality (output1 == pivot) violation
  EXPECT_GE(solver.numVars(), 6);
  // Dummy Objective
  EXPECT_LE(solver.numVars(), 7);
  // 2 Linear
  // 1 from breaking the cycle (output1 == pivot)
  EXPECT_EQ(solver.numInvariants(), 2 + 1);
}

TEST(InvariantGraphTest, BreakElementIndexCycle) {
  propagation::Solver solver;
  InvariantGraph invariantGraph(solver);
  /* Graph:
   *
   *     x11   x12    x21   x22
   *       |   |        |   |
   *       v   v        v   v
   *      +-----+      +-----+
   *  +-->| pl1 |  +-->| pl2 |
   *  |   +-----+  |   +-----+
   *  |      |     |      |
   *  |      v     |      v
   *  |   output1--+   output2
   *  |                   |
   *  +-------------------+
   */

  const VarNodeId x11 = invariantGraph.retrieveIntVarNode(SearchDomain(0, 1));
  const VarNodeId x12 = invariantGraph.retrieveIntVarNode(SearchDomain(0, 1));
  const VarNodeId x21 = invariantGraph.retrieveIntVarNode(SearchDomain(0, 1));
  const VarNodeId x22 = invariantGraph.retrieveIntVarNode(SearchDomain(0, 1));
  const VarNodeId output1 =
      invariantGraph.retrieveIntVarNode(SearchDomain(0, 1));
  const VarNodeId output2 =
      invariantGraph.retrieveIntVarNode(SearchDomain(0, 1));

  invariantGraph.addInvariantNode(std::make_shared<ArrayVarElementNode>(
      invariantGraph, output2, std::vector<VarNodeId>{x11, x12}, output1, 0));

  invariantGraph.addInvariantNode(std::make_shared<ArrayVarElementNode>(
      invariantGraph, output1, std::vector<VarNodeId>{x21, x22}, output2, 0));

  invariantGraph.apply();
  invariantGraph.close();

  // x11, x12, x21, x22, output1, output1
  // the pivot
  // The Equality violation
  // dummy objective
  EXPECT_EQ(solver.numVars(), 6 + 1 + 1 + 1);
  // 2 Element
  // 1 Total Violation
  // 1 from breaking the cycle
  EXPECT_EQ(solver.numInvariants(), 2 + 1);
}

TEST(InvariantGraphTest, AllowDynamicCycle) {
  propagation::Solver solver;
  InvariantGraph invariantGraph(solver);
  /* Graph:
   *
   *             +---------------------+
   *             |                     |
   *         x1  |   +----------+   x2 |
   *         |   |   |          |   |  |
   *         v   v   |          v   v  |
   *        +-----+  |        +-----+  |
   * idx1-->| el1 |  | idx2-->| el2 |  |
   *        +-----+  |        +-----+  |
   *           |     |           |     |
   *           v     |           v     |
   *        output1--+        output2--+
   *
   */

  const VarNodeId idx1 = invariantGraph.retrieveIntVarNode(SearchDomain(1, 2));
  const VarNodeId x1 = invariantGraph.retrieveIntVarNode(SearchDomain(0, 10));
  const VarNodeId idx2 = invariantGraph.retrieveIntVarNode(SearchDomain(1, 2));
  const VarNodeId x2 = invariantGraph.retrieveIntVarNode(SearchDomain(0, 10));
  const VarNodeId output1 =
      invariantGraph.retrieveIntVarNode(SearchDomain(0, 10));
  const VarNodeId output2 =
      invariantGraph.retrieveIntVarNode(SearchDomain(0, 10));

  invariantGraph.addInvariantNode(std::make_shared<ArrayVarElementNode>(
      invariantGraph, idx1, std::vector<VarNodeId>{x1, output2}, output1, 1));

  invariantGraph.addInvariantNode(std::make_shared<ArrayVarElementNode>(
      invariantGraph, idx2, std::vector<VarNodeId>{output1, x2}, output2, 1));

  invariantGraph.apply();
  invariantGraph.close();

  // idx1, x1, idx2, x2, output1, output2
  EXPECT_GE(solver.numVars(), 6);
  // dummy objective + dummy violation
  EXPECT_LE(solver.numVars(), 8);
  // 2 Element
  // (no violations)
  EXPECT_EQ(solver.numInvariants(), 2);
}
}  // namespace atlantis::testing
