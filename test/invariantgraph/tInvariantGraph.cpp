#include <gtest/gtest.h>

#include <string>

#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/invariantGraphRoot.hpp"
#include "invariantgraph/invariantNodes/arrayVarElementNode.hpp"
#include "invariantgraph/invariantNodes/intLinearNode.hpp"
#include "invariantgraph/invariantNodes/intPlusNode.hpp"
#include "propagation/solver.hpp"
#include "utils/domains.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

TEST(InvariantGraphTest, apply_result) {
  fznparser::Model model;
  InvariantGraph invariantGraph;

  const VarNodeId a =
      invariantGraph.createVarNode(SearchDomain(0, 10), true, "a", false);
  const VarNodeId b =
      invariantGraph.createVarNode(SearchDomain(0, 10), true, "b", false);
  const VarNodeId output =
      invariantGraph.createVarNode(SearchDomain(0, 10), true, "output", true);

  EXPECT_TRUE(invariantGraph.containsVarNode("a"));
  EXPECT_TRUE(invariantGraph.containsVarNode("b"));
  EXPECT_TRUE(invariantGraph.containsVarNode("output"));

  invariantGraph.addInvariantNode(
      std::make_unique<IntPlusNode>(a, b, output));

  invariantGraph.addImplicitConstraintNode(std::move(
      std::make_unique<InvariantGraphRoot>(std::vector<VarNodeId>{a, b})));

  propagation::Solver solver;
  invariantGraph.apply(solver);

  EXPECT_NE(invariantGraph.varNode(a).varId(), propagation::NULL_ID);
  EXPECT_NE(invariantGraph.varNode(b).varId(), propagation::NULL_ID);
  EXPECT_NE(invariantGraph.varNode(output).varId(), propagation::NULL_ID);
}

TEST(InvariantGraphTest, ApplyGraph) {
  InvariantGraph invariantGraph;

  const VarNodeId a1 =
      invariantGraph.createVarNode(SearchDomain(0, 10), true, "a1", false);
  const VarNodeId a2 =
      invariantGraph.createVarNode(SearchDomain(0, 10), true, "a2", false);
  const VarNodeId b1 =
      invariantGraph.createVarNode(SearchDomain(0, 10), true, "b1", false);
  const VarNodeId b2 =
      invariantGraph.createVarNode(SearchDomain(0, 10), true, "b2", false);

  const VarNodeId output1 =
      invariantGraph.createVarNode(SearchDomain(0, 20), true, "output1", true);
  const VarNodeId output2 =
      invariantGraph.createVarNode(SearchDomain(0, 20), true, "output2", true);

  const VarNodeId output3 =
      invariantGraph.createVarNode(SearchDomain(0, 40), true, "output3", true);

  invariantGraph.addInvariantNode(
      std::make_unique<IntPlusNode>(a1, a2, output1));
  invariantGraph.addInvariantNode(
      std::make_unique<IntPlusNode>(b1, b2, output2));
  invariantGraph.addInvariantNode(
      std::make_unique<IntPlusNode>(output1, output2, output3));

  propagation::Solver solver;
  invariantGraph.apply(solver);

  // 7 variables + dummy objective
  EXPECT_EQ(solver.numVars(), 8);
  EXPECT_EQ(solver.numInvariants(), 3);
}

TEST(InvariantGraphTest, SplitSimpleGraph) {
  fznparser::Model model;
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
  InvariantGraph invariantGraph;

  const VarNodeId a =
      invariantGraph.createVarNode(SearchDomain(0, 10), true, "a", false);
  const VarNodeId b =
      invariantGraph.createVarNode(SearchDomain(0, 10), true, "b", false);
  const VarNodeId c =
      invariantGraph.createVarNode(SearchDomain(0, 10), true, "c", false);
  const VarNodeId d =
      invariantGraph.createVarNode(SearchDomain(0, 10), true, "d", false);
  const VarNodeId output =
      invariantGraph.createVarNode(SearchDomain(0, 20), true, "output", true);

  invariantGraph.addInvariantNode(std::make_unique<IntPlusNode>(a, b, output));

  invariantGraph.addInvariantNode(std::make_unique<IntPlusNode>(c, d, output));

  propagation::Solver solver;
  invariantGraph.apply(solver);

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
  fznparser::Model model;
  InvariantGraph invariantGraph;
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
  const VarNodeId output = invariantGraph.createVarNode(
      SearchDomain(lb * numInputs, ub * numInputs), true, "output", true);

  std::vector<std::vector<VarNodeId>> varNodeIdMatrix(numInvariants,
                                                      std::vector<VarNodeId>{});
  for (size_t i = 0; i < numInvariants; ++i) {
    for (size_t j = 0; j < numInputs; ++j) {
      const std::string identifier(std::to_string(i) + "_" + std::to_string(j));
      varNodeIdMatrix.at(i).emplace_back(invariantGraph.createVarNode(
          SearchDomain(lb, ub), true, std::string(identifier), false));
    }
  }

  std::vector<Int> coeffs(numInputs, 1);
  for (const auto& identifierArray : varNodeIdMatrix) {
    std::vector<VarNodeId> inputs(identifierArray);
    invariantGraph.addInvariantNode(std::make_unique<IntLinearNode>(
        std::vector<Int>(coeffs), std::move(inputs), output));
  }

  propagation::Solver solver;
  invariantGraph.apply(solver);

  // Each invariant has numInputs inputs
  // Each invariant has 1 output
  // There is one violation for the AllDiff
  // One view for the AllDiff
  EXPECT_EQ(solver.numVars(), numInvariants * (numInputs + 1) + 2);
  EXPECT_EQ(solver.numInvariants(), numInvariants + 1);
}

TEST(InvariantGraphTest, BreakSimpleCycle) {
  fznparser::Model model;
  InvariantGraph invariantGraph;
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

  const VarNodeId x1 =
      invariantGraph.createVarNode(SearchDomain(0, 10), true, "x1", false);
  const VarNodeId x2 =
      invariantGraph.createVarNode(SearchDomain(0, 10), true, "x2", false);
  const VarNodeId output1 =
      invariantGraph.createVarNode(SearchDomain(0, 10), true, "output1", true);
  const VarNodeId output2 =
      invariantGraph.createVarNode(SearchDomain(0, 10), true, "output2", true);

  invariantGraph.addInvariantNode(
      std::make_unique<IntPlusNode>(x1, output2, output1));

  invariantGraph.addInvariantNode(
      std::make_unique<IntPlusNode>(output1, x2, output2));

  propagation::Solver solver;
  invariantGraph.apply(solver);

  // x1, x2, output1, output2
  // the pivot
  // The Equality (output1 == pivot) violation
  // Dummy Objective
  EXPECT_EQ(solver.numVars(), 4 + 1 + 1 + 1);
  // 2 Linear
  // 1 from breaking the cycle (output1 == pivot)
  EXPECT_EQ(solver.numInvariants(), 2 + 1);
}

TEST(InvariantGraphTest, BreakElementIndexCycle) {
  fznparser::Model model;
  InvariantGraph invariantGraph;
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

  const VarNodeId x11 =
      invariantGraph.createVarNode(SearchDomain(0, 1), true, "x11", false);
  const VarNodeId x12 =
      invariantGraph.createVarNode(SearchDomain(0, 1), true, "x12", false);
  const VarNodeId x21 =
      invariantGraph.createVarNode(SearchDomain(0, 1), true, "x21", false);
  const VarNodeId x22 =
      invariantGraph.createVarNode(SearchDomain(0, 1), true, "x22", false);
  const VarNodeId output1 =
      invariantGraph.createVarNode(SearchDomain(0, 1), true, "output1", true);
  const VarNodeId output2 =
      invariantGraph.createVarNode(SearchDomain(0, 1), true, "output2", true);

  invariantGraph.addInvariantNode(
      std::make_unique<ArrayVarElementNode>(
          output2, std::vector<VarNodeId>{x11, x12}, output1, 0));

  invariantGraph.addInvariantNode(
      std::make_unique<ArrayVarElementNode>(
          output1, std::vector<VarNodeId>{x21, x22}, output2, 0));

  propagation::Solver solver;
  invariantGraph.apply(solver);

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
  fznparser::Model model;
  InvariantGraph invariantGraph;
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

  const VarNodeId idx1 =
      invariantGraph.createVarNode(SearchDomain(1, 2), true, "idx1", false);
  const VarNodeId x1 =
      invariantGraph.createVarNode(SearchDomain(0, 10), true, "x1", false);
  const VarNodeId idx2 =
      invariantGraph.createVarNode(SearchDomain(1, 2), true, "idx2", false);
  const VarNodeId x2 =
      invariantGraph.createVarNode(SearchDomain(0, 10), true, "x2", false);
  const VarNodeId output1 =
      invariantGraph.createVarNode(SearchDomain(0, 10), true, "output1", true);
  const VarNodeId output2 =
      invariantGraph.createVarNode(SearchDomain(0, 10), true, "output2", true);

  invariantGraph.addInvariantNode(
      std::make_unique<ArrayVarElementNode>(
          idx1, std::vector<VarNodeId>{x1, output2}, output1, 1));

  invariantGraph.addInvariantNode(
      std::make_unique<ArrayVarElementNode>(
          idx2, std::vector<VarNodeId>{output1, x2}, output2, 1));

  propagation::Solver solver;
  invariantGraph.apply(solver);

  // idx1, x1, idx2, x2, output1, output2
  // dummy objective
  // (no violations)
  EXPECT_EQ(solver.numVars(), 6 + 1);
  // 2 Element
  // (no violations)
  EXPECT_EQ(solver.numInvariants(), 2);
}
}  // namespace atlantis::testing