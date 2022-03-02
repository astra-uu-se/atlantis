#include "invariantgraph/constraints/intNeNode.hpp"

#include "constraints/notEqual.hpp"
#include "invariantgraph/parseHelper.hpp"

std::unique_ptr<invariantgraph::IntNeNode>
invariantgraph::IntNeNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint>& constraint,
    const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
        variableMap) {
  assert(constraint->name() == "int_ne");
  assert(constraint->arguments().size() == 2);

  MAPPED_SEARCH_VARIABLE_ARG(a, constraint->arguments()[0], variableMap);
  MAPPED_SEARCH_VARIABLE_ARG(b, constraint->arguments()[1], variableMap);

  auto node = std::make_unique<IntNeNode>(a, b);
  a->addSoftConstraint(node.get());
  b->addSoftConstraint(node.get());

  return node;
}

VarId invariantgraph::IntNeNode::registerWithEngine(
    Engine& engine, std::function<VarId(VariableNode*)> variableMapper) const {
  VarId violation = engine.makeIntVar(0, 0, 1);

  engine.makeConstraint<::NotEqual>(violation, variableMapper(_a),
                                 variableMapper(_b));

  return violation;
}