#include <gtest/gtest.h>

#include "core/propagationEngine.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantGraphRoot.hpp"
#include "invariantgraph/invariantNodes/arrayVarIntElementNode.hpp"
#include "invariantgraph/invariantNodes/intLinearNode.hpp"

TEST(InvariantGraphTest, apply_result) {
  fznparser::Model model;
  invariantgraph::InvariantGraph invariantGraph;

  const fznparser::IntVar& a = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 10, "a")));
  const fznparser::IntVar& b = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 10, "b")));
  const fznparser::IntVar& c = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 10, "c")));

  auto aNode = invariantGraph.createVarNode(a);
  auto bNode = invariantGraph.createVarNode(b);
  auto cNode = invariantGraph.createVarNode(c);

  auto plusNode = std::make_unique<invariantgraph::IntLinearNode>(
      std::vector<Int>{1, 1},
      std::vector<invariantgraph::VarNodeId>{aNode, bNode}, cNode, -1, 0);

  auto root = std::make_unique<invariantgraph::InvariantGraphRoot>(
      std::vector<invariantgraph::VarNodeId>{aNode, bNode});

  std::vector<invariantgraph::VarNodeId> variableNodes;
  variableNodes.push_back(aNode);
  variableNodes.push_back(bNode);
  variableNodes.push_back(cNode);

  std::vector<std::unique_ptr<invariantgraph::InvariantNode>> variableDefiningNodes;
  variableDefiningNodes.push_back(std::move(plusNode));
  variableDefiningNodes.push_back(std::move(root));

  PropagationEngine engine;
  auto result = invariantGraph.apply(engine);

  EXPECT_EQ(result.variableIdentifiers().size(), 3);
  std::unordered_set<std::string_view> varNames;
  for (const auto& [varId, var] : result.variableIdentifiers()) {
    varNames.emplace(var);
  }
  EXPECT_TRUE(varNames.contains(a.identifier()));
  EXPECT_TRUE(varNames.contains(b.identifier()));
  EXPECT_TRUE(varNames.contains(c.identifier()));
}

TEST(InvariantGraphTest, ApplyGraph) {
  fznparser::Model model;
  invariantgraph::InvariantGraph invariantGraph;
  const fznparser::IntVar& a1 = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 10, "a1")));
  const fznparser::IntVar& a2 = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 10, "a2")));

  const fznparser::IntVar& b1 = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 10, "b1")));
  const fznparser::IntVar& b2 = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 10, "b2")));

  const fznparser::IntVar& c1 = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 20, "c1")));
  const fznparser::IntVar& c2 = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 20, "c2")));

  const fznparser::IntVar& sum = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 40, "sum")));

  auto a1Node = invariantGraph.createVarNode(a1);
  auto a2Node = invariantGraph.createVarNode(a2);

  auto b1Node = invariantGraph.createVarNode(b1);
  auto b2Node = invariantGraph.createVarNode(b2);

  auto c1Node = invariantGraph.createVarNode(c1);
  auto c2Node = invariantGraph.createVarNode(c2);

  auto sumNode = invariantGraph.createVarNode(sum);

  auto aPlusNode = std::make_unique<invariantgraph::IntLinearNode>(
      std::vector<Int>{1, 1},
      std::vector<invariantgraph::VarNodeId>{a1Node, a2Node}, c1Node, -1, 0);

  auto bPlusNode = std::make_unique<invariantgraph::IntLinearNode>(
      std::vector<Int>{1, 1},
      std::vector<invariantgraph::VarNodeId>{b1Node, b2Node}, c2Node, -1, 0);

  auto cPlusNode = std::make_unique<invariantgraph::IntLinearNode>(
      std::vector<Int>{1, 1},
      std::vector<invariantgraph::VarNodeId>{c1Node, c2Node}, sumNode, -1, 0);

  auto root = std::make_unique<invariantgraph::InvariantGraphRoot>(
      std::vector<invariantgraph::VarNodeId>{a1Node, a2Node, b1Node, b2Node});

  std::vector<invariantgraph::VarNodeId> variableNodes;
  variableNodes.push_back(std::move(a1Node));
  variableNodes.push_back(std::move(a2Node));
  variableNodes.push_back(std::move(b1Node));
  variableNodes.push_back(std::move(b2Node));
  variableNodes.push_back(std::move(c1Node));
  variableNodes.push_back(std::move(c2Node));
  variableNodes.push_back(std::move(sumNode));

  std::vector<std::unique_ptr<invariantgraph::InvariantNode>> variableDefiningNodes;

  variableDefiningNodes.push_back(std::move(aPlusNode));
  variableDefiningNodes.push_back(std::move(bPlusNode));
  variableDefiningNodes.push_back(std::move(cPlusNode));
  variableDefiningNodes.push_back(std::move(root));

  PropagationEngine engine;
  auto result = invariantGraph.apply(engine);

  EXPECT_EQ(result.variableIdentifiers().size(), 7);
  // 7 variables + dummy objective
  EXPECT_EQ(engine.numVariables(), 8);
  EXPECT_EQ(engine.numInvariants(), 3);
}

TEST(InvariantGraphTest, SplitSimpleGraph) {
  fznparser::Model model;
  invariantgraph::InvariantGraph invariantGraph;
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

  const fznparser::IntVar& a = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 10, "a")));
  const fznparser::IntVar& b = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 10, "b")));
  const fznparser::IntVar& c = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 10, "c")));
  const fznparser::IntVar& d = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 10, "d")));
  const fznparser::IntVar& x = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 20, "x")));

  auto aNode = invariantGraph.createVarNode(a);
  auto bNode = invariantGraph.createVarNode(b);
  auto cNode = invariantGraph.createVarNode(c);
  auto dNode = invariantGraph.createVarNode(d);
  auto xNode = invariantGraph.createVarNode(x);

  auto plusNode1 = std::make_unique<invariantgraph::IntLinearNode>(
      std::vector<Int>{1, 1},
      std::vector<invariantgraph::VarNodeId>{aNode, bNode}, xNode, -1, 0);

  auto plusNode2 = std::make_unique<invariantgraph::IntLinearNode>(
      std::vector<Int>{1, 1},
      std::vector<invariantgraph::VarNodeId>{cNode, dNode}, xNode, -1, 0);

  auto root = std::make_unique<invariantgraph::InvariantGraphRoot>(
      std::vector<invariantgraph::VarNodeId>{aNode, bNode, cNode, dNode});

  std::vector<invariantgraph::VarNodeId> variableNodes;
  variableNodes.push_back(std::move(aNode));
  variableNodes.push_back(std::move(bNode));
  variableNodes.push_back(std::move(cNode));
  variableNodes.push_back(std::move(dNode));
  variableNodes.push_back(std::move(xNode));

  std::vector<std::unique_ptr<invariantgraph::InvariantNode>> variableDefiningNodes;

  variableDefiningNodes.push_back(std::move(plusNode1));
  variableDefiningNodes.push_back(std::move(plusNode2));
  variableDefiningNodes.push_back(std::move(root));

  PropagationEngine engine;
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
  invariantgraph::InvariantGraph invariantGraph;
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
  const fznparser::IntVar& x = std::get<fznparser::IntVar>(model.createVarNode(
      fznparser::IntVar(lb * numInputs, ub * numInputs, "x")));
  auto xNode = invariantGraph.createVarNode(x);
  std::vector<std::vector<invariantgraph::VarNodeId>> inputNodes;
  std::vector<Int> coeffs(numInputs, 1);
  std::vector<std::unique_ptr<invariantgraph::InvariantNode>> variableDefiningNodes;

  for (size_t i = 0; i < numInvariants; ++i) {
    inputNodes.emplace_back(std::vector<invariantgraph::VarNodeId>{});

    std::vector<invariantgraph::VarNodeId> arguments;
    for (size_t j = 0; j < numInputs; ++j) {
      const fznparser::IntVar& input =
          std::get<fznparser::IntVar>(model.createVarNode(fznparser::IntVar(
              0, 10, std ::to_string(i) + "_" + std::to_string(j))));

      arguments.emplace_back(
          inputNodes.back().emplace_back(invariantGraph.createVarNode(input)));
    }
    variableDefiningNodes.emplace_back(
        std::make_unique<invariantgraph::IntLinearNode>(
            std::vector<Int>(coeffs),
            std::vector<invariantgraph::VarNodeId>(arguments), xNode, -1, 0));
  }

  std::vector<invariantgraph::VarNodeId> searchVariables;
  for (const auto& inv : inputNodes) {
    for (const auto& input : inv) {
      searchVariables.emplace_back(input);
    }
  }

  auto root =
      std::make_unique<invariantgraph::InvariantGraphRoot>(searchVariables);

  std::vector<invariantgraph::VarNodeId> variableNodes;
  for (auto& inv : inputNodes) {
    for (auto& input : inv) {
      variableNodes.push_back(std::move(input));
    }
  }
  variableNodes.push_back(std::move(xNode));

  variableDefiningNodes.push_back(std::move(root));

  PropagationEngine engine;
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
  invariantgraph::InvariantGraph invariantGraph;
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

  const fznparser::IntVar& a = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 10, "a")));
  const fznparser::IntVar& b = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 10, "b")));
  const fznparser::IntVar& x = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 10, "x")));
  const fznparser::IntVar& y = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 10, "y")));

  auto aNode = invariantGraph.createVarNode(a);
  auto bNode = invariantGraph.createVarNode(b);
  auto xNode = invariantGraph.createVarNode(x);
  auto yNode = invariantGraph.createVarNode(y);

  auto plusNode1 = std::make_unique<invariantgraph::IntLinearNode>(
      std::vector<Int>{1, 1},
      std::vector<invariantgraph::VarNodeId>{aNode, yNode}, xNode, -1, 0);

  auto plusNode2 = std::make_unique<invariantgraph::IntLinearNode>(
      std::vector<Int>{1, 1},
      std::vector<invariantgraph::VarNodeId>{xNode, bNode}, yNode, -1, 0);

  auto root = std::make_unique<invariantgraph::InvariantGraphRoot>(
      std::vector<invariantgraph::VarNodeId>{aNode, bNode});

  std::vector<invariantgraph::VarNodeId> variableNodes;
  variableNodes.push_back(aNode);
  variableNodes.push_back(bNode);
  variableNodes.push_back(xNode);
  variableNodes.push_back(yNode);

  std::vector<std::unique_ptr<invariantgraph::InvariantNode>> variableDefiningNodes;

  variableDefiningNodes.push_back(std::move(plusNode1));
  variableDefiningNodes.push_back(std::move(plusNode2));
  variableDefiningNodes.push_back(std::move(root));

  invariantGraph.breakCycles();

  PropagationEngine engine;
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
  invariantgraph::InvariantGraph invariantGraph;
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

  const fznparser::IntVar& a = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 10, "a")));
  const fznparser::IntVar& b = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 10, "b")));
  const fznparser::IntVar& c = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 10, "c")));
  const fznparser::IntVar& d = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 10, "d")));
  const fznparser::IntVar& x = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(1, 2, "x")));
  const fznparser::IntVar& y = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(1, 2, "y")));

  auto aNode = invariantGraph.createVarNode(a);
  auto bNode = invariantGraph.createVarNode(b);
  auto cNode = invariantGraph.createVarNode(c);
  auto dNode = invariantGraph.createVarNode(d);
  auto xNode = invariantGraph.createVarNode(x);
  auto yNode = invariantGraph.createVarNode(y);

  auto elementNode1 = std::make_unique<invariantgraph::ArrayVarIntElementNode>(
      yNode, std::vector<invariantgraph::VarNodeId>{aNode, bNode}, xNode, 1);

  auto elementNode2 = std::make_unique<invariantgraph::ArrayVarIntElementNode>(
      xNode, std::vector<invariantgraph::VarNodeId>{cNode, dNode}, yNode, 1);

  auto root = std::make_unique<invariantgraph::InvariantGraphRoot>(
      std::vector<invariantgraph::VarNodeId>{aNode, bNode, cNode, dNode});

  std::vector<invariantgraph::VarNodeId> variableNodes;
  variableNodes.push_back(aNode);
  variableNodes.push_back(bNode);
  variableNodes.push_back(cNode);
  variableNodes.push_back(dNode);
  variableNodes.push_back(xNode);
  variableNodes.push_back(yNode);

  std::vector<std::unique_ptr<invariantgraph::InvariantNode>> variableDefiningNodes;

  variableDefiningNodes.push_back(std::move(elementNode1));
  variableDefiningNodes.push_back(std::move(elementNode2));
  variableDefiningNodes.push_back(std::move(root));

  invariantGraph.breakCycles();

  PropagationEngine engine;
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
  invariantgraph::InvariantGraph invariantGraph;
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

  const fznparser::IntVar& a = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(1, 2, "a")));
  const fznparser::IntVar& b = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 10, "b")));
  const fznparser::IntVar& c = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(1, 2, "c")));
  const fznparser::IntVar& d = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 10, "d")));
  const fznparser::IntVar& x = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 10, "x")));
  const fznparser::IntVar& y = std::get<fznparser::IntVar>(
      model.createVarNode(fznparser::IntVar(0, 10, "y")));

  auto aNode = invariantGraph.createVarNode(a);
  auto bNode = invariantGraph.createVarNode(b);
  auto cNode = invariantGraph.createVarNode(c);
  auto dNode = invariantGraph.createVarNode(d);
  auto xNode = invariantGraph.createVarNode(x);
  auto yNode = invariantGraph.createVarNode(y);

  auto elementNode1 = std::make_unique<invariantgraph::ArrayVarIntElementNode>(
      aNode, std::vector<invariantgraph::VarNodeId>{bNode, yNode}, xNode, 1);

  auto elementNode2 = std::make_unique<invariantgraph::ArrayVarIntElementNode>(
      cNode, std::vector<invariantgraph::VarNodeId>{xNode, dNode}, yNode, 1);

  auto root = std::make_unique<invariantgraph::InvariantGraphRoot>(
      std::vector<invariantgraph::VarNodeId>{aNode, bNode, cNode, dNode});

  std::vector<invariantgraph::VarNodeId> variableNodes;
  variableNodes.push_back(aNode);
  variableNodes.push_back(bNode);
  variableNodes.push_back(cNode);
  variableNodes.push_back(dNode);
  variableNodes.push_back(xNode);
  variableNodes.push_back(yNode);

  std::vector<std::unique_ptr<invariantgraph::InvariantNode>> variableDefiningNodes;

  variableDefiningNodes.push_back(std::move(elementNode1));
  variableDefiningNodes.push_back(std::move(elementNode2));
  variableDefiningNodes.push_back(std::move(root));

  invariantGraph.breakCycles();

  PropagationEngine engine;
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