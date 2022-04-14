#include <gtest/gtest.h>

#include "core/propagationEngine.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantGraphRoot.hpp"
#include "invariantgraph/invariants/linearNode.hpp"

TEST(InvariantGraphTest, apply_result) {
  auto a = fznparser::IntVariable{"a", fznparser::IntRange{0, 10}, {}, 0};
  auto b = fznparser::IntVariable{"b", fznparser::IntRange{0, 10}, {}, 0};
  auto c = fznparser::IntVariable{"c", fznparser::IntRange{0, 10}, {}, 0};

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

  invariantgraph::InvariantGraph graph(
      std::move(variableNodes), std::move(variableDefiningNodes), cNode.get());

  PropagationEngine engine;
  auto result = graph.apply(engine);

  EXPECT_EQ(result.variableMap().size(), 3);
}
