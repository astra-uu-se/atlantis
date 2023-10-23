#include <gtest/gtest.h>

#include <string>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantGraphRoot.hpp"
#include "invariantgraph/invariantNodes/arrayVarIntElementNode.hpp"
#include "invariantgraph/invariantNodes/intLinearNode.hpp"
#include "propagation/propagationEngine.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

TEST(InvariantGraphTest, apply_result) {
  fznparser::Model model;
  InvariantGraph invariantGraph;

  const fznparser::IntVar& a = std::get<fznparser::IntVar>(
      model.addVariable(fznparser::IntVar(0, 10, "a")));
  const fznparser::IntVar& b = std::get<fznparser::IntVar>(
      model.addVariable(fznparser::IntVar(0, 10, "b")));
  const fznparser::IntVar& c = std::get<fznparser::IntVar>(
      model.addVariable(fznparser::IntVar(0, 10, "c")));

  auto aNode = invariantGraph.createVarNode(a);
  auto bNode = invariantGraph.createVarNode(b);
  auto cNode = invariantGraph.createVarNode(c);

  invariantGraph.addInvariantNode(std::move(std::make_unique<IntLinearNode>(
      std::vector<Int>{1, 1}, std::vector<VarNodeId>{aNode, bNode}, cNode, -1,
      0)));

  invariantGraph.addImplicitConstraintNode(
      std::move(std::make_unique<InvariantGraphRoot>(
          std::vector<VarNodeId>{aNode, bNode})));

  propagation::PropagationEngine engine;
  auto result = invariantGraph.apply(engine);

  EXPECT_EQ(result.variableIdentifiers().size(), 3);
  std::unordered_set<std::string> varNames;
  for (const auto& [varId, var] : result.variableIdentifiers()) {
    varNames.emplace(var);
  }
  EXPECT_TRUE(varNames.contains(a.identifier()));
  EXPECT_TRUE(varNames.contains(b.identifier()));
  EXPECT_TRUE(varNames.contains(c.identifier()));
}

TEST(InvariantGraphTest, ApplyGraph) {
  fznparser::Model model;
  const std::string a1 = std::get<fznparser::IntVar>(
                             model.addVariable(fznparser::IntVar(0, 10, "a1")))
                             .identifier();
  const std::string a2 = std::get<fznparser::IntVar>(
                             model.addVariable(fznparser::IntVar(0, 10, "a2")))
                             .identifier();
  const std::string b1 = std::get<fznparser::IntVar>(
                             model.addVariable(fznparser::IntVar(0, 10, "b1")))
                             .identifier();
  const std::string b2 = std::get<fznparser::IntVar>(
                             model.addVariable(fznparser::IntVar(0, 10, "b2")))
                             .identifier();

  const std::string c1 = std::get<fznparser::IntVar>(
                             model.addVariable(fznparser::IntVar(0, 20, "c1")))
                             .identifier();
  const std::string c2 = std::get<fznparser::IntVar>(
                             model.addVariable(fznparser::IntVar(0, 20, "c2")))
                             .identifier();

  const std::string sum =
      std::get<fznparser::IntVar>(
          model.addVariable(fznparser::IntVar(0, 40, "sum")))
          .identifier();

  InvariantGraph invariantGraph;
  const VarNodeId a1NodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(a1)));
  const VarNodeId a2NodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(a2)));

  const VarNodeId b1NodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(b1)));
  const VarNodeId b2NodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(b2)));

  const VarNodeId c1NodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(c1)));
  const VarNodeId c2NodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(c2)));

  const VarNodeId sumNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(sum)));

  invariantGraph.addInvariantNode(std::move(std::make_unique<IntLinearNode>(
      std::vector<Int>{1, 1}, std::vector<VarNodeId>{a1NodeId, a2NodeId},
      c1NodeId, -1, 0)));
  invariantGraph.addInvariantNode(std::move(std::make_unique<IntLinearNode>(
      std::vector<Int>{1, 1}, std::vector<VarNodeId>{b1NodeId, b2NodeId},
      c2NodeId, -1, 0)));
  invariantGraph.addInvariantNode(std::move(std::make_unique<IntLinearNode>(
      std::vector<Int>{1, 1}, std::vector<VarNodeId>{c1NodeId, c2NodeId},
      sumNodeId, -1, 0)));

  propagation::PropagationEngine engine;
  auto result = invariantGraph.apply(engine);

  EXPECT_EQ(result.variableIdentifiers().size(), 7);
  // 7 variables + dummy objective
  EXPECT_EQ(engine.numVariables(), 8);
  EXPECT_EQ(engine.numInvariants(), 3);
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
   *         x
   *
   */

  const std::string a = std::get<fznparser::IntVar>(
                            model.addVariable(fznparser::IntVar(0, 10, "a")))
                            .identifier();
  const std::string b = std::get<fznparser::IntVar>(
                            model.addVariable(fznparser::IntVar(0, 10, "b")))
                            .identifier();
  const std::string c = std::get<fznparser::IntVar>(
                            model.addVariable(fznparser::IntVar(0, 10, "c")))
                            .identifier();
  const std::string d = std::get<fznparser::IntVar>(
                            model.addVariable(fznparser::IntVar(0, 10, "d")))
                            .identifier();
  const std::string x = std::get<fznparser::IntVar>(
                            model.addVariable(fznparser::IntVar(0, 20, "x")))
                            .identifier();

  InvariantGraph invariantGraph;
  const VarNodeId aNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(a)));
  const VarNodeId bNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(b)));
  const VarNodeId cNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(c)));
  const VarNodeId dNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(d)));
  const VarNodeId xNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(x)));

  invariantGraph.addInvariantNode(std::make_unique<IntLinearNode>(
      std::vector<Int>{1, 1}, std::vector<VarNodeId>{aNodeId, bNodeId}, xNodeId,
      -1, 0));
  invariantGraph.addInvariantNode(std::make_unique<IntLinearNode>(
      std::vector<Int>{1, 1}, std::vector<VarNodeId>{cNodeId, dNodeId}, xNodeId,
      -1, 0));

  propagation::PropagationEngine engine;
  auto result = invariantGraph.apply(engine);

  EXPECT_EQ(result.variableIdentifiers().size(), 5);
  // a, b, c, d, x
  // x_copy
  // violation for equal constraint (x == x_copy)
  // dummy objective
  EXPECT_EQ(engine.numVariables(), 5 + 1 + 1 + 1);
  // 2 Linear
  // 1 equal
  EXPECT_EQ(engine.numInvariants(), 3);
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
   *         x
   *
   */

  const size_t numInvariants = 5;
  const size_t numInputs = 5;
  const Int lb = 0;
  const Int ub = 10;
  const std::string x =
      model.addVariable(fznparser::IntVar(lb * numInputs, ub * numInputs, "x"))
          .identifier();

  std::vector<std::vector<std::string>> identifierMatrix(
      numInvariants, std::vector<std::string>{});
  for (size_t i = 0; i < numInvariants; ++i) {
    for (size_t j = 0; j < numInputs; ++j) {
      const std::string identifier(std::to_string(i) + "_" + std::to_string(j));
      identifierMatrix.at(i).emplace_back(std::string(identifier));
      model.addVariable(
          std::move(fznparser::IntVar(lb, ub, std::string(identifier))));
    }
  }

  const VarNodeId xNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(x)));

  std::vector<Int> coeffs(numInputs, 1);
  for (const auto& identifierArray : identifierMatrix) {
    std::vector<VarNodeId> inputs;
    for (const auto& identifier : identifierArray) {
      inputs.emplace_back(invariantGraph.createVarNode(
          std::get<fznparser::IntVar>(model.variable(identifier))));
    }
    invariantGraph.addInvariantNode(std::move(std::make_unique<IntLinearNode>(
        std::vector<Int>(coeffs), std::move(inputs), xNodeId, -1, 0)));
  }

  propagation::PropagationEngine engine;
  auto result = invariantGraph.apply(engine);

  EXPECT_EQ(result.variableIdentifiers().size(), numInvariants * numInputs + 1);
  // Each invariant has numInputs inputs
  // Each invariant has 1 output
  // There is one violation for the AllDiff
  // One view for the AllDiff
  EXPECT_EQ(engine.numVariables(), numInvariants * (numInputs + 1) + 2);
  EXPECT_EQ(engine.numInvariants(), numInvariants + 1);
}

TEST(InvariantGraphTest, BreakSimpleCycle) {
  fznparser::Model model;
  InvariantGraph invariantGraph;
  /* Graph:
   *
   *      +---------------+
   *      |               |
   *  a   |   +---+   b   |
   *  |   |   |   |   |   |
   *  v   v   |   v   v   |
   * +-----+  |  +-----+  |
   * | pl1 |  |  | pl2 |  |
   * +-----+  |  +-----+  |
   *    |     |     |     |
   *    v     |     v     |
   *    x-----+     y-----+
   *
   */

  const std::string a = std::get<fznparser::IntVar>(
                            model.addVariable(fznparser::IntVar(0, 10, "a")))
                            .identifier();
  const std::string b = std::get<fznparser::IntVar>(
                            model.addVariable(fznparser::IntVar(0, 10, "b")))
                            .identifier();
  const std::string x = std::get<fznparser::IntVar>(
                            model.addVariable(fznparser::IntVar(0, 10, "x")))
                            .identifier();
  const std::string y = std::get<fznparser::IntVar>(
                            model.addVariable(fznparser::IntVar(0, 10, "y")))
                            .identifier();

  const VarNodeId aNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(a)));
  const VarNodeId bNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(b)));
  const VarNodeId xNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(x)));
  const VarNodeId yNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(y)));

  invariantGraph.addInvariantNode(std::move(std::make_unique<IntLinearNode>(
      std::vector<Int>{1, 1}, std::vector<VarNodeId>{aNodeId, yNodeId}, xNodeId,
      -1, 0)));

  invariantGraph.addInvariantNode(std::move(std::make_unique<IntLinearNode>(
      std::vector<Int>{1, 1}, std::vector<VarNodeId>{xNodeId, bNodeId}, yNodeId,
      -1, 0)));

  propagation::PropagationEngine engine;
  auto result = invariantGraph.apply(engine);

  EXPECT_EQ(result.variableIdentifiers().size(), 4);
  // a, b, x, y
  // the pivot
  // The Equality (x == pivot) violation
  // total violation
  // Dummy Objective
  EXPECT_EQ(engine.numVariables(), 4 + 1 + 1 + 1 + 1);
  // 2 Linear
  // 1 from breaking the cycle (x == pivot)
  // 1 Total Violation
  EXPECT_EQ(engine.numInvariants(), 2 + 1 + 1);
}

TEST(InvariantGraphTest, BreakElementIndexCycle) {
  fznparser::Model model;
  InvariantGraph invariantGraph;
  /* Graph:
   *
   *       a   b        c   d
   *       |   |        |   |
   *       v   v        v   v
   *      +-----+      +-----+
   *  +-->| pl1 |  +-->| pl2 |
   *  |   +-----+  |   +-----+
   *  |      |     |      |
   *  |      v     |      v
   *  |      x-----+      y
   *  |                   |
   *  +-------------------+
   */

  const std::string a = std::get<fznparser::IntVar>(
                            model.addVariable(fznparser::IntVar(0, 10, "a")))
                            .identifier();
  const std::string b = std::get<fznparser::IntVar>(
                            model.addVariable(fznparser::IntVar(0, 10, "b")))
                            .identifier();
  const std::string c = std::get<fznparser::IntVar>(
                            model.addVariable(fznparser::IntVar(0, 10, "c")))
                            .identifier();
  const std::string d = std::get<fznparser::IntVar>(
                            model.addVariable(fznparser::IntVar(0, 10, "d")))
                            .identifier();
  const std::string x = std::get<fznparser::IntVar>(
                            model.addVariable(fznparser::IntVar(1, 2, "x")))
                            .identifier();
  const std::string y = std::get<fznparser::IntVar>(
                            model.addVariable(fznparser::IntVar(1, 2, "y")))
                            .identifier();

  const VarNodeId aNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(a)));
  const VarNodeId bNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(b)));
  const VarNodeId cNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(c)));
  const VarNodeId dNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(d)));
  const VarNodeId xNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(x)));
  const VarNodeId yNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(y)));

  invariantGraph.addInvariantNode(
      std::move(std::make_unique<ArrayVarIntElementNode>(
          yNodeId, std::vector<VarNodeId>{aNodeId, bNodeId}, xNodeId, 1)));

  invariantGraph.addInvariantNode(
      std::move(std::make_unique<ArrayVarIntElementNode>(
          xNodeId, std::vector<VarNodeId>{cNodeId, dNodeId}, yNodeId, 1)));

  propagation::PropagationEngine engine;
  auto result = invariantGraph.apply(engine);

  EXPECT_EQ(result.variableIdentifiers().size(), 6);
  // a, b, c, d, x, y
  // the pivot
  // The Equality violation
  // total violation
  // dummy objective
  EXPECT_EQ(engine.numVariables(), 6 + 1 + 1 + 1 + 1);
  // 2 Element
  // 1 Total Violation
  // 1 from breaking the cycle
  EXPECT_EQ(engine.numInvariants(), 2 + 1 + 1);
}

TEST(InvariantGraphTest, AllowDynamicCycle) {
  fznparser::Model model;
  InvariantGraph invariantGraph;
  /* Graph:
   *
   *          +-------------------+
   *          |                   |
   *      b   |   +--------+   d  |
   *      |   |   |        |   |  |
   *      v   v   |        v   v  |
   *     +-----+  |      +-----+  |
   * a-->| el1 |  |  c-->| el2 |  |
   *     +-----+  |      +-----+  |
   *        |     |         |     |
   *        v     |         v     |
   *        x-----+         y-----+
   *
   */

  const std::string a = std::get<fznparser::IntVar>(
                            model.addVariable(fznparser::IntVar(1, 2, "a")))
                            .identifier();
  const std::string b = std::get<fznparser::IntVar>(
                            model.addVariable(fznparser::IntVar(0, 10, "b")))
                            .identifier();
  const std::string c = std::get<fznparser::IntVar>(
                            model.addVariable(fznparser::IntVar(1, 2, "c")))
                            .identifier();
  const std::string d = std::get<fznparser::IntVar>(
                            model.addVariable(fznparser::IntVar(0, 10, "d")))
                            .identifier();
  const std::string x = std::get<fznparser::IntVar>(
                            model.addVariable(fznparser::IntVar(0, 10, "x")))
                            .identifier();
  const std::string y = std::get<fznparser::IntVar>(
                            model.addVariable(fznparser::IntVar(0, 10, "y")))
                            .identifier();

  const VarNodeId aNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(a)));
  const VarNodeId bNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(b)));
  const VarNodeId cNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(c)));
  const VarNodeId dNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(d)));
  const VarNodeId xNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(x)));
  const VarNodeId yNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntVar>(model.variable(y)));

  invariantGraph.addInvariantNode(
      std::move(std::make_unique<ArrayVarIntElementNode>(
          aNodeId, std::vector<VarNodeId>{bNodeId, yNodeId}, xNodeId, 1)));

  invariantGraph.addInvariantNode(
      std::move(std::make_unique<ArrayVarIntElementNode>(
          cNodeId, std::vector<VarNodeId>{xNodeId, dNodeId}, yNodeId, 1)));

  propagation::PropagationEngine engine;
  auto result = invariantGraph.apply(engine);

  EXPECT_EQ(result.variableIdentifiers().size(), 6);
  // a, b, c, d, x, y
  // dummy objective
  // (no violations)
  EXPECT_EQ(engine.numVariables(), 6 + 1);
  // 2 Element
  // (no violations)
  EXPECT_EQ(engine.numInvariants(), 2);
}
}  // namespace atlantis::testing