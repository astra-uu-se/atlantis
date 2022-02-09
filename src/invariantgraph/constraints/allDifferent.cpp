#include "invariantgraph/constraints/allDifferent.hpp"

#include <utility>

#include "constraints/allDifferent.hpp"

std::shared_ptr<invariantgraph::AllDifferentNode>
invariantgraph::AllDifferentNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint>& constraint,
    const std::function<std::shared_ptr<VariableNode>(
        std::shared_ptr<fznparser::Variable>)>& variableMap) {
  std::vector<std::shared_ptr<VariableNode>> variableNodes;
  std::transform(
      constraint->arguments().begin(), constraint->arguments().end(),
      std::back_inserter(variableNodes), [&variableMap](auto arg) {
        auto literal = std::get<std::shared_ptr<fznparser::Literal>>(arg);
        assert(literal->type() == fznparser::LiteralType::SEARCH_VARIABLE);

        auto variable = std::dynamic_pointer_cast<fznparser::Variable>(literal);
        return variableMap(variable);
      });

  auto node = std::make_shared<AllDifferentNode>(variableNodes);
  for (const auto& variableNode : variableNodes)
    variableNode->addSoftConstraint(node);

  return node;
}

VarId invariantgraph::AllDifferentNode::registerWithEngine(
    Engine& engine,
    std::function<VarId(const std::shared_ptr<VariableNode>&)> variableMapper)
    const {
  VarId violation = engine.makeIntVar(0, 0, _variables.size());

  std::vector<VarId> engineVariables;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(engineVariables), variableMapper);

  engine.makeConstraint<AllDifferent>(violation, engineVariables);

  return violation;
}