#include "invariantgraph/violationInvariantNodes/intLtNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<IntLtNode> IntLtNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2 ||
      constraint.arguments().size() != 3) {
    throw std::runtime_error("IntLe constraint takes two var bool arguments");
  }

  VarNodeId a = invariantGraph.createVarNode(
      std::get<fznparser::IntArg>(constraint.arguments().at(0)));

  VarNodeId b = invariantGraph.createVarNode(
      std::get<fznparser::IntArg>(constraint.arguments().at(1)));

  if (constraint.arguments().size() == 2) {
    return std::make_unique<IntLtNode>(a, b, true);
  }

  const auto& reified = get<fznparser::BoolArg>(constraint.arguments().at(2));
  if (reified.isFixed()) {
    return std::make_unique<IntLtNode>(a, b, reified.toParameter());
  }
  return std::make_unique<IntLtNode>(
      a, b, invariantGraph.createVarNode(reified.var()));
}

void IntLtNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                        Engine& engine) {
  registerViolation(engine);
}

void IntLtNode::registerNode(InvariantGraph& invariantGraph, Engine& engine) {
  assert(violationVarId() != NULL_ID);

  if (shouldHold()) {
    engine.makeConstraint<LessThan>(engine, violationVarId(), a()->varId(),
                                    b()->varId());
  } else {
    assert(!isReified());
    engine.makeConstraint<LessEqual>(engine, violationVarId(), b()->varId(),
                                     a()->varId());
  }
}

}  // namespace invariantgraph