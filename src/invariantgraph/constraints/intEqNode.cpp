#include "invariantgraph/constraints/intEqNode.hpp"

#include "constraints/equal.hpp"
#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::IntEqNode>
invariantgraph::IntEqNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint>& constraint,
    const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
        variableMap) {
  MAPPED_SEARCH_VARIABLE_ARG(a, constraint->arguments()[0], variableMap);
  MAPPED_SEARCH_VARIABLE_ARG(b, constraint->arguments()[1], variableMap);

  return std::make_unique<IntEqNode>(a, b);
}

void invariantgraph::IntEqNode::registerWithEngine(
    Engine& engine, std::map<VariableNode *, VarId> &variableMap) {
  VarId violation = registerViolation(engine, variableMap);

  engine.makeConstraint<::Equal>(violation, variableMap.at(_a),
                                 variableMap.at(_b));
}