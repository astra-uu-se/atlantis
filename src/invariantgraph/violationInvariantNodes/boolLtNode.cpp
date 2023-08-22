#include "invariantgraph/violationInvariantNodes/boolLtNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<BoolLtNode> BoolLtNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2 ||
      constraint.arguments().size() != 3) {
    throw std::runtime_error("BoolLt constraint takes two var bool arguments");
  }
  for (const auto& arg : constraint.arguments()) {
    if (!std::holds_alternative<fznparser::BoolArg>(arg)) {
      throw std::runtime_error(
          "BoolLt constraint takes two var bool arguments");
    }
  }

  VarNodeId a = invariantGraph.createVarNode(
      std::get<fznparser::BoolArg>(constraint.arguments().at(0)));

  VarNodeId b = invariantGraph.createVarNode(
      std::get<fznparser::BoolArg>(constraint.arguments().at(1)));

  if (constraint.arguments().size() == 2) {
    return std::make_unique<BoolLtNode>(a, b, true);
  }

  const auto& reified = get<fznparser::BoolArg>(constraint.arguments().at(2));
  if (reified.isFixed()) {
    return std::make_unique<BoolLtNode>(a, b, reified.toParameter());
  }
  return std::make_unique<BoolLtNode>(
      a, b, invariantGraph.createVarNode(reified.var()));
}

void BoolLtNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                         Engine& engine) {
  registerViolation(engine);
}

void BoolLtNode::registerNode(InvariantGraph& invariantGraph, Engine& engine) {
  assert(violationVarId() != NULL_ID);
  assert(a()->varId() != NULL_ID);
  assert(b()->varId() != NULL_ID);

  if (shouldHold()) {
    engine.makeConstraint<BoolLessThan>(engine, violationVarId(), a()->varId(),
                                        b()->varId());
  } else {
    assert(!isReified());
    engine.makeConstraint<BoolLessEqual>(engine, violationVarId(), b()->varId(),
                                         a()->varId());
  }
}

}  // namespace invariantgraph