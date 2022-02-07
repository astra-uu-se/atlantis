#include <gtest/gtest.h>

#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/linear.hpp"

TEST(LinearInvariantNode, variable_mapper_is_called_for_all_vars) {
  PropagationEngine engine;
  engine.open();
  auto a = std::make_shared<invariantgraph::VariableNode>();
  auto b = std::make_shared<invariantgraph::VariableNode>();
  std::vector<std::shared_ptr<invariantgraph::VariableNode>> vars{a, b};

  invariantgraph::LinearInvariantNode linearInvariant({1, 1}, vars);

  std::vector<std::shared_ptr<invariantgraph::VariableNode>> mappedNodes;
  linearInvariant.registerWithEngine(engine,
                                     [&mappedNodes, &engine](auto node) {
                                       mappedNodes.emplace_back(node);
                                       return engine.makeIntVar(0, 0, 1);
                                     });

  engine.close();
  EXPECT_EQ(mappedNodes, vars);
}