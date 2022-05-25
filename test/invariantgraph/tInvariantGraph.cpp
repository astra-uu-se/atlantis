#include <gtest/gtest.h>

#include "core/propagationEngine.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantGraphRoot.hpp"
#include "invariantgraph/invariants/arrayVarIntElementNode.hpp"
#include "invariantgraph/invariants/intLinearNode.hpp"

TEST(InvariantGraphTest, apply_result) {
  fznparser::IntVariable a{"a", fznparser::IntRange{0, 10}, {}, {}};
  fznparser::IntVariable b{"b", fznparser::IntRange{0, 10}, {}, {}};
  fznparser::IntVariable c{"c", fznparser::IntRange{0, 10}, {}, {}};

  auto aNode = std::make_unique<invariantgraph::VariableNode>(a);
  auto bNode = std::make_unique<invariantgraph::VariableNode>(b);
  auto cNode = std::make_unique<invariantgraph::VariableNode>(c);

  auto plusNode = std::make_unique<invariantgraph::IntLinearNode>(
      std::vector<Int>{1, 1},
      std::vector<invariantgraph::VariableNode*>{aNode.get(), bNode.get()},
      cNode.get(), -1, 0);

  auto root = std::make_unique<invariantgraph::InvariantGraphRoot>(
      std::vector<invariantgraph::VariableNode*>{aNode.get(), bNode.get()});

  std::vector<std::unique_ptr<invariantgraph::VariableNode>> variableNodes;
  variableNodes.push_back(std::move(aNode));
  variableNodes.push_back(std::move(bNode));
  variableNodes.push_back(std::move(cNode));

  std::vector<std::unique_ptr<invariantgraph::VariableDefiningNode>>
      variableDefiningNodes;
  variableDefiningNodes.push_back(std::move(plusNode));
  variableDefiningNodes.push_back(std::move(root));

  invariantgraph::InvariantGraph graph(std::move(variableNodes), {},
                                       std::move(variableDefiningNodes),
                                       cNode.get());

  PropagationEngine engine;
  auto result = graph.apply(engine);

  EXPECT_EQ(result.variableMap().size(), 3);
  std::unordered_set<std::string> varNames;
  for (const auto& [varId, var] : result.variableMap()) {
    varNames.emplace(var);
  }
  EXPECT_TRUE(varNames.contains(a.name));
  EXPECT_TRUE(varNames.contains(b.name));
  EXPECT_TRUE(varNames.contains(c.name));
}

TEST(InvariantGraphTest, ApplyGraph) {
  fznparser::IntVariable a1{"a1", fznparser::IntRange{0, 10}, {}, {}};
  fznparser::IntVariable a2{"a2", fznparser::IntRange{0, 10}, {}, {}};

  fznparser::IntVariable b1{"b1", fznparser::IntRange{0, 10}, {}, {}};
  fznparser::IntVariable b2{"b2", fznparser::IntRange{0, 10}, {}, {}};

  fznparser::IntVariable c1{"c1", fznparser::IntRange{0, 20}, {}, {}};
  fznparser::IntVariable c2{"c2", fznparser::IntRange{0, 20}, {}, {}};

  fznparser::IntVariable sum{"sum", fznparser::IntRange{0, 40}, {}, {}};

  auto a1Node = std::make_unique<invariantgraph::VariableNode>(a1);
  auto a2Node = std::make_unique<invariantgraph::VariableNode>(a2);

  auto b1Node = std::make_unique<invariantgraph::VariableNode>(b1);
  auto b2Node = std::make_unique<invariantgraph::VariableNode>(b2);

  auto c1Node = std::make_unique<invariantgraph::VariableNode>(c1);
  auto c2Node = std::make_unique<invariantgraph::VariableNode>(c2);

  auto sumNode = std::make_unique<invariantgraph::VariableNode>(sum);

  auto aPlusNode = std::make_unique<invariantgraph::IntLinearNode>(
      std::vector<Int>{1, 1},
      std::vector<invariantgraph::VariableNode*>{a1Node.get(), a2Node.get()},
      c1Node.get(), -1, 0);

  auto bPlusNode = std::make_unique<invariantgraph::IntLinearNode>(
      std::vector<Int>{1, 1},
      std::vector<invariantgraph::VariableNode*>{b1Node.get(), b2Node.get()},
      c2Node.get(), -1, 0);

  auto cPlusNode = std::make_unique<invariantgraph::IntLinearNode>(
      std::vector<Int>{1, 1},
      std::vector<invariantgraph::VariableNode*>{c1Node.get(), c2Node.get()},
      sumNode.get(), -1, 0);

  auto root = std::make_unique<invariantgraph::InvariantGraphRoot>(
      std::vector<invariantgraph::VariableNode*>{a1Node.get(), a2Node.get(),
                                                 b1Node.get(), b2Node.get()});

  std::vector<std::unique_ptr<invariantgraph::VariableNode>> variableNodes;
  variableNodes.push_back(std::move(a1Node));
  variableNodes.push_back(std::move(a2Node));
  variableNodes.push_back(std::move(b1Node));
  variableNodes.push_back(std::move(b2Node));
  variableNodes.push_back(std::move(c1Node));
  variableNodes.push_back(std::move(c2Node));
  variableNodes.push_back(std::move(sumNode));

  std::vector<std::unique_ptr<invariantgraph::VariableDefiningNode>>
      variableDefiningNodes;

  variableDefiningNodes.push_back(std::move(aPlusNode));
  variableDefiningNodes.push_back(std::move(bPlusNode));
  variableDefiningNodes.push_back(std::move(cPlusNode));
  variableDefiningNodes.push_back(std::move(root));

  invariantgraph::InvariantGraph graph(std::move(variableNodes), {},
                                       std::move(variableDefiningNodes),
                                       sumNode.get());

  PropagationEngine engine;
  auto result = graph.apply(engine);

  EXPECT_EQ(result.variableMap().size(), 7);
  // 7 variables + dummy objective
  EXPECT_EQ(engine.numVariables(), 8);
  EXPECT_EQ(engine.numInvariants(), 3);
}

TEST(InvariantGraphTest, SplitSimpleGraph) {
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

  fznparser::IntVariable a{"a", fznparser::IntRange{0, 10}, {}, {}};
  fznparser::IntVariable b{"b", fznparser::IntRange{0, 10}, {}, {}};
  fznparser::IntVariable c{"c", fznparser::IntRange{0, 10}, {}, {}};
  fznparser::IntVariable d{"d", fznparser::IntRange{0, 10}, {}, {}};
  fznparser::IntVariable x{"x", fznparser::IntRange{0, 20}, {}, {}};

  auto aNode = std::make_unique<invariantgraph::VariableNode>(a);
  auto bNode = std::make_unique<invariantgraph::VariableNode>(b);
  auto cNode = std::make_unique<invariantgraph::VariableNode>(c);
  auto dNode = std::make_unique<invariantgraph::VariableNode>(d);
  auto xNode = std::make_unique<invariantgraph::VariableNode>(x);

  auto plusNode1 = std::make_unique<invariantgraph::IntLinearNode>(
      std::vector<Int>{1, 1},
      std::vector<invariantgraph::VariableNode*>{aNode.get(), bNode.get()},
      xNode.get(), -1, 0);

  auto plusNode2 = std::make_unique<invariantgraph::IntLinearNode>(
      std::vector<Int>{1, 1},
      std::vector<invariantgraph::VariableNode*>{cNode.get(), dNode.get()},
      xNode.get(), -1, 0);

  auto root = std::make_unique<invariantgraph::InvariantGraphRoot>(
      std::vector<invariantgraph::VariableNode*>{aNode.get(), bNode.get(),
                                                 cNode.get(), dNode.get()});

  std::vector<std::unique_ptr<invariantgraph::VariableNode>> variableNodes;
  variableNodes.push_back(std::move(aNode));
  variableNodes.push_back(std::move(bNode));
  variableNodes.push_back(std::move(cNode));
  variableNodes.push_back(std::move(dNode));
  variableNodes.push_back(std::move(xNode));

  std::vector<std::unique_ptr<invariantgraph::VariableDefiningNode>>
      variableDefiningNodes;

  variableDefiningNodes.push_back(std::move(plusNode1));
  variableDefiningNodes.push_back(std::move(plusNode2));
  variableDefiningNodes.push_back(std::move(root));

  invariantgraph::InvariantGraph graph(
      std::move(variableNodes), {}, std::move(variableDefiningNodes), nullptr);

  PropagationEngine engine;
  auto result = graph.apply(engine);

  EXPECT_EQ(result.variableMap().size(), 5);
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
  fznparser::IntVariable x{
      "x", fznparser::IntRange{lb * numInputs, ub * numInputs}, {}, {}};
  auto xNode = std::make_unique<invariantgraph::VariableNode>(x);
  std::vector<std::vector<std::unique_ptr<invariantgraph::VariableNode>>>
      inputNodes;
  std::vector<Int> coeffs(numInputs, 1);
  std::vector<std::unique_ptr<invariantgraph::VariableDefiningNode>>
      variableDefiningNodes;

  for (size_t i = 0; i < numInvariants; ++i) {
    inputNodes.emplace_back(
        std::vector<std::unique_ptr<invariantgraph::VariableNode>>());
    std::vector<invariantgraph::VariableNode*> arguments;
    for (size_t j = 0; j < numInputs; ++j) {
      fznparser::IntVariable input{std::to_string(i) + "_" + std::to_string(j),
                                   fznparser::IntRange{0, 10},
                                   {},
                                   {}};

      arguments.emplace_back(
          inputNodes.back()
              .emplace_back(
                  std::make_unique<invariantgraph::VariableNode>(input))
              .get());
    }
    variableDefiningNodes.emplace_back(
        std::make_unique<invariantgraph::IntLinearNode>(coeffs, arguments,
                                                        xNode.get(), -1, 0));
  }

  std::vector<invariantgraph::VariableNode*> searchVariables;
  for (const auto& inv : inputNodes) {
    for (const auto& input : inv) {
      searchVariables.emplace_back(input.get());
    }
  }

  auto root =
      std::make_unique<invariantgraph::InvariantGraphRoot>(searchVariables);

  std::vector<std::unique_ptr<invariantgraph::VariableNode>> variableNodes;
  for (auto& inv : inputNodes) {
    for (auto& input : inv) {
      variableNodes.push_back(std::move(input));
    }
  }
  variableNodes.push_back(std::move(xNode));

  variableDefiningNodes.push_back(std::move(root));

  invariantgraph::InvariantGraph graph(
      std::move(variableNodes), {}, std::move(variableDefiningNodes), nullptr);

  PropagationEngine engine;
  auto result = graph.apply(engine);

  EXPECT_EQ(result.variableMap().size(), numInvariants * numInputs + 1);
  // Each invariant has numInputs inputs
  // Each invariant has 1 output
  // There is one violation for the AllDiff
  // One view for the AllDiff
  EXPECT_EQ(engine.numVariables(), numInvariants * (numInputs + 1) + 2);
  EXPECT_EQ(engine.numInvariants(), numInvariants + 1);
}

TEST(InvariantGraphTest, BreakSimpleCycle) {
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

  fznparser::IntVariable a{"a", fznparser::IntRange{0, 10}, {}, {}};
  fznparser::IntVariable b{"b", fznparser::IntRange{0, 10}, {}, {}};
  fznparser::IntVariable x{"x", fznparser::IntRange{0, 10}, {}, {}};
  fznparser::IntVariable y{"y", fznparser::IntRange{0, 10}, {}, {}};

  auto aNode = std::make_unique<invariantgraph::VariableNode>(a);
  auto bNode = std::make_unique<invariantgraph::VariableNode>(b);
  auto xNode = std::make_unique<invariantgraph::VariableNode>(x);
  auto yNode = std::make_unique<invariantgraph::VariableNode>(y);

  auto plusNode1 = std::make_unique<invariantgraph::IntLinearNode>(
      std::vector<Int>{1, 1},
      std::vector<invariantgraph::VariableNode*>{aNode.get(), yNode.get()},
      xNode.get(), -1, 0);

  auto plusNode2 = std::make_unique<invariantgraph::IntLinearNode>(
      std::vector<Int>{1, 1},
      std::vector<invariantgraph::VariableNode*>{xNode.get(), bNode.get()},
      yNode.get(), -1, 0);

  auto root = std::make_unique<invariantgraph::InvariantGraphRoot>(
      std::vector<invariantgraph::VariableNode*>{aNode.get(), bNode.get()});

  std::vector<std::unique_ptr<invariantgraph::VariableNode>> variableNodes;
  variableNodes.push_back(std::move(aNode));
  variableNodes.push_back(std::move(bNode));
  variableNodes.push_back(std::move(xNode));
  variableNodes.push_back(std::move(yNode));

  std::vector<std::unique_ptr<invariantgraph::VariableDefiningNode>>
      variableDefiningNodes;

  variableDefiningNodes.push_back(std::move(plusNode1));
  variableDefiningNodes.push_back(std::move(plusNode2));
  variableDefiningNodes.push_back(std::move(root));

  invariantgraph::InvariantGraph graph(
      std::move(variableNodes), {}, std::move(variableDefiningNodes), nullptr);

  graph.breakCycles();

  PropagationEngine engine;
  auto result = graph.apply(engine);

  EXPECT_EQ(result.variableMap().size(), 4);
  // a, b, x, y
  // domain constraints for x and y
  // the pivot
  // The Equality (x == pivot) violation
  // total violation
  // Dummy Objective
  EXPECT_EQ(engine.numVariables(), 4 + 2 + 1 + 1 + 1 + 1);
  // 2 Linear
  // 1 from breaking the cycle (x == pivot)
  // 2 domain constraints
  // 1 Total Violation
  EXPECT_EQ(engine.numInvariants(), 2 + 2 + 1 + 1);
}

TEST(InvariantGraphTest, BreakElementIndexCycle) {
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

  fznparser::IntVariable a{"a", fznparser::IntRange{0, 10}, {}, {}};
  fznparser::IntVariable b{"b", fznparser::IntRange{0, 10}, {}, {}};
  fznparser::IntVariable c{"c", fznparser::IntRange{0, 10}, {}, {}};
  fznparser::IntVariable d{"d", fznparser::IntRange{0, 10}, {}, {}};
  fznparser::IntVariable x{"x", fznparser::IntRange{1, 2}, {}, {}};
  fznparser::IntVariable y{"y", fznparser::IntRange{1, 2}, {}, {}};

  auto aNode = std::make_unique<invariantgraph::VariableNode>(a);
  auto bNode = std::make_unique<invariantgraph::VariableNode>(b);
  auto cNode = std::make_unique<invariantgraph::VariableNode>(c);
  auto dNode = std::make_unique<invariantgraph::VariableNode>(d);
  auto xNode = std::make_unique<invariantgraph::VariableNode>(x);
  auto yNode = std::make_unique<invariantgraph::VariableNode>(y);

  auto elementNode1 = std::make_unique<invariantgraph::ArrayVarIntElementNode>(
      yNode.get(),
      std::vector<invariantgraph::VariableNode*>{aNode.get(), bNode.get()},
      xNode.get());

  auto elementNode2 = std::make_unique<invariantgraph::ArrayVarIntElementNode>(
      xNode.get(),
      std::vector<invariantgraph::VariableNode*>{cNode.get(), dNode.get()},
      yNode.get());

  auto root = std::make_unique<invariantgraph::InvariantGraphRoot>(
      std::vector<invariantgraph::VariableNode*>{aNode.get(), bNode.get(),
                                                 cNode.get(), dNode.get()});

  std::vector<std::unique_ptr<invariantgraph::VariableNode>> variableNodes;
  variableNodes.push_back(std::move(aNode));
  variableNodes.push_back(std::move(bNode));
  variableNodes.push_back(std::move(cNode));
  variableNodes.push_back(std::move(dNode));
  variableNodes.push_back(std::move(xNode));
  variableNodes.push_back(std::move(yNode));

  std::vector<std::unique_ptr<invariantgraph::VariableDefiningNode>>
      variableDefiningNodes;

  variableDefiningNodes.push_back(std::move(elementNode1));
  variableDefiningNodes.push_back(std::move(elementNode2));
  variableDefiningNodes.push_back(std::move(root));

  invariantgraph::InvariantGraph graph(
      std::move(variableNodes), {}, std::move(variableDefiningNodes), nullptr);

  graph.breakCycles();

  PropagationEngine engine;
  auto result = graph.apply(engine);

  EXPECT_EQ(result.variableMap().size(), 6);
  // a, b, c, d, x, y
  // domain constraints for x and y
  // the pivot
  // The Equality violation
  // total violation
  // dummy objective
  EXPECT_EQ(engine.numVariables(), 6 + 2 + 1 + 1 + 1 + 1);
  // 2 Element
  // 2 domain constraints
  // 1 Total Violation
  // 1 from breaking the cycle
  EXPECT_EQ(engine.numInvariants(), 2 + 2 + 1 + 1);
}

TEST(InvariantGraphTest, AllowDynamicCycle) {
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

  fznparser::IntVariable a{"a", fznparser::IntRange{1, 2}, {}, {}};
  fznparser::IntVariable b{"b", fznparser::IntRange{0, 10}, {}, {}};
  fznparser::IntVariable c{"c", fznparser::IntRange{1, 2}, {}, {}};
  fznparser::IntVariable d{"d", fznparser::IntRange{0, 10}, {}, {}};
  fznparser::IntVariable x{"x", fznparser::IntRange{0, 10}, {}, {}};
  fznparser::IntVariable y{"y", fznparser::IntRange{0, 10}, {}, {}};

  auto aNode = std::make_unique<invariantgraph::VariableNode>(a);
  auto bNode = std::make_unique<invariantgraph::VariableNode>(b);
  auto cNode = std::make_unique<invariantgraph::VariableNode>(c);
  auto dNode = std::make_unique<invariantgraph::VariableNode>(d);
  auto xNode = std::make_unique<invariantgraph::VariableNode>(x);
  auto yNode = std::make_unique<invariantgraph::VariableNode>(y);

  auto elementNode1 = std::make_unique<invariantgraph::ArrayVarIntElementNode>(
      aNode.get(),
      std::vector<invariantgraph::VariableNode*>{bNode.get(), yNode.get()},
      xNode.get());

  auto elementNode2 = std::make_unique<invariantgraph::ArrayVarIntElementNode>(
      cNode.get(),
      std::vector<invariantgraph::VariableNode*>{xNode.get(), dNode.get()},
      yNode.get());

  auto root = std::make_unique<invariantgraph::InvariantGraphRoot>(
      std::vector<invariantgraph::VariableNode*>{aNode.get(), bNode.get(),
                                                 cNode.get(), dNode.get()});

  std::vector<std::unique_ptr<invariantgraph::VariableNode>> variableNodes;
  variableNodes.push_back(std::move(aNode));
  variableNodes.push_back(std::move(bNode));
  variableNodes.push_back(std::move(cNode));
  variableNodes.push_back(std::move(dNode));
  variableNodes.push_back(std::move(xNode));
  variableNodes.push_back(std::move(yNode));

  std::vector<std::unique_ptr<invariantgraph::VariableDefiningNode>>
      variableDefiningNodes;

  variableDefiningNodes.push_back(std::move(elementNode1));
  variableDefiningNodes.push_back(std::move(elementNode2));
  variableDefiningNodes.push_back(std::move(root));

  invariantgraph::InvariantGraph graph(
      std::move(variableNodes), {}, std::move(variableDefiningNodes), nullptr);

  graph.breakCycles();

  PropagationEngine engine;
  auto result = graph.apply(engine);

  EXPECT_EQ(result.variableMap().size(), 6);
  // a, b, c, d, x, y
  // dummy objective
  // (no violations)
  EXPECT_EQ(engine.numVariables(), 6 + 1);
  // 2 Element
  // (no violations)
  EXPECT_EQ(engine.numInvariants(), 2);
}