#include "invariantgraph/constraints/allDifferentNode.hpp"

#include <utility>

#include "constraints/allDifferent.hpp"

std::unique_ptr<invariantgraph::AllDifferentNode>
invariantgraph::AllDifferentNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint>& constraint,
    const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
        variableMap) {
  std::vector<VariableNode*> variableNodes;
  std::transform(
      constraint->arguments().begin(), constraint->arguments().end(),
      std::back_inserter(variableNodes), [&variableMap](auto arg) {
        auto literal = std::get<std::shared_ptr<fznparser::Literal>>(arg);
        assert(literal->type() == fznparser::LiteralType::SEARCH_VARIABLE);

        auto variable = std::dynamic_pointer_cast<fznparser::Variable>(literal);
        return variableMap(variable);
      });

  auto node = std::make_unique<AllDifferentNode>(variableNodes);
  for (auto variableNode : variableNodes) {
    variableNode->addSoftConstraint(node.get());
  }

  return node;
}

VarId invariantgraph::AllDifferentNode::registerWithEngine(
    Engine& engine, std::function<VarId(VariableNode*)> variableMapper) const {
  VarId violation = engine.makeIntVar(0, 0, _variables.size());

  std::vector<VarId> engineVariables;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(engineVariables), variableMapper);

  engine.makeConstraint<AllDifferent>(violation, engineVariables);

  return violation;
}