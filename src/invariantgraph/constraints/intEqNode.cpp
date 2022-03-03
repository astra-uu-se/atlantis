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

  auto node = std::make_unique<IntEqNode>(a, b);
  a->addSoftConstraint(node.get());
  b->addSoftConstraint(node.get());

  return node;
}

VarId invariantgraph::IntEqNode::registerWithEngine(
    Engine& engine, std::function<VarId(VariableNode*)> variableMapper) const {
  auto al = _a->variable()->domain()->lowerBound();
  auto au = _a->variable()->domain()->upperBound();
  auto bl = _b->variable()->domain()->lowerBound();
  auto bu = _b->variable()->domain()->upperBound();

  auto diff1 = std::abs(al - bu);
  auto diff2 = std::abs(au - bl);
  VarId violation = engine.makeIntVar(0, 0, std::max(diff1, diff2));

  engine.makeConstraint<::Equal>(violation, variableMapper(_a),
                                 variableMapper(_b));

  return violation;
}