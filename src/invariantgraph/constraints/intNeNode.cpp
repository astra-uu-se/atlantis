#include "invariantgraph/constraints/intNeNode.hpp"

#include "../parseHelper.hpp"
#include "constraints/notEqual.hpp"

std::unique_ptr<invariantgraph::IntNeNode>
invariantgraph::IntNeNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint>& constraint,
    const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
        variableMap) {
  assert(constraint->name() == "int_ne");
  assert(constraint->arguments().size() == 2);

  MAPPED_SEARCH_VARIABLE_ARG(a, constraint->arguments()[0], variableMap);
  MAPPED_SEARCH_VARIABLE_ARG(b, constraint->arguments()[1], variableMap);

  return std::make_unique<IntNeNode>(a, b);
}

void invariantgraph::IntNeNode::registerWithEngine(
    Engine& engine, std::map<VariableNode*, VarId>& variableMap) {
  VarId violation = registerViolation(engine, variableMap);

  engine.makeConstraint<::NotEqual>(violation, variableMap.at(_a),
                                    variableMap.at(_b));
}