#include "invariantgraph/violationInvariantNodes/intNeNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<IntNeNode> IntNeNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2 ||
      constraint.arguments().size() != 3) {
    throw std::runtime_error("IntNe constraint takes two var bool arguments");
  }

  VarNodeId a = invariantGraph.createVarNode(
      std::get<fznparser::IntArg>(constraint.arguments().at(0)));

  VarNodeId b = invariantGraph.createVarNode(
      std::get<fznparser::IntArg>(constraint.arguments().at(1)));

  if (constraint.arguments().size() == 2) {
    return std::make_unique<IntNeNode>(a, b, true);
  }

  const auto& reified = get<fznparser::BoolArg>(constraint.arguments().at(2));
  if (reified.isFixed()) {
    return std::make_unique<IntNeNode>(a, b, reified.toParameter());
  }
  return std::make_unique<IntNeNode>(
      a, b, invariantGraph.createVarNode(reified.var()));
}

void IntNeNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                        Engine& engine) {
  registerViolation(engine);
}

void IntNeNode::registerNode(InvariantGraph& invariantGraph, Engine& engine) {
  assert(violationVarId() != NULL_ID);

  if (shouldHold()) {
    engine.makeConstraint<NotEqual>(engine, violationVarId(), a()->varId(),
                                    b()->varId());
  } else {
    assert(!isReified());
    engine.makeConstraint<Equal>(engine, violationVarId(), a()->varId(),
                                 b()->varId());
  }
}

}  // namespace invariantgraph