#include <gtest/gtest.h>

#include "core/propagationEngine.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantGraphRoot.hpp"
#include "invariantgraph/invariants/linearNode.hpp"

TEST(InvariantGraphTest, apply_result) {
  fznparser::IntVariable a{"a", fznparser::IntRange{0, 10}, {}, {}};
  fznparser::IntVariable b{"b", fznparser::IntRange{0, 10}, {}, {}};
  fznparser::IntVariable c{"c", fznparser::IntRange{0, 10}, {}, {}};

  auto aNode = std::make_unique<invariantgraph::VariableNode>(a);
  auto bNode = std::make_unique<invariantgraph::VariableNode>(b);
  auto cNode = std::make_unique<invariantgraph::VariableNode>(c);

  auto plusNode = std::make_unique<invariantgraph::LinearNode>(
      std::vector<Int>{1, 1},
      std::vector<invariantgraph::VariableNode*>{aNode.get(), bNode.get()},
      cNode.get());

  auto root = std::make_unique<invariantgraph::InvariantGraphRoot>(
      std::vector<invariantgraph::VariableNode*>{aNode.get(), bNode.get()});

  std::vector<std::unique_ptr<invariantgraph::VariableNode>> variableNodes;
  variableNodes.push_back(std::move(aNode));
  variableNodes.push_back(std::move(bNode));

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

  auto aPlusNode = std::make_unique<invariantgraph::LinearNode>(
      std::vector<Int>{1, 1},
      std::vector<invariantgraph::VariableNode*>{a1Node.get(), a2Node.get()},
      c1Node.get());

  auto bPlusNode = std::make_unique<invariantgraph::LinearNode>(
      std::vector<Int>{1, 1},
      std::vector<invariantgraph::VariableNode*>{b1Node.get(), b2Node.get()},
      c2Node.get());

  auto cPlusNode = std::make_unique<invariantgraph::LinearNode>(
      std::vector<Int>{1, 1},
      std::vector<invariantgraph::VariableNode*>{c1Node.get(), c2Node.get()},
      sumNode.get());

  auto root = std::make_unique<invariantgraph::InvariantGraphRoot>(
      std::vector<invariantgraph::VariableNode*>{a1Node.get(), a2Node.get(),
                                                 b1Node.get(), b2Node.get()});

  std::vector<std::unique_ptr<invariantgraph::VariableNode>> variableNodes;
  variableNodes.push_back(std::move(a1Node));
  variableNodes.push_back(std::move(a2Node));
  variableNodes.push_back(std::move(b1Node));
  variableNodes.push_back(std::move(b2Node));

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
  EXPECT_EQ(engine.numVariables(), 8);
  EXPECT_EQ(engine.numInvariants(), 4);
}