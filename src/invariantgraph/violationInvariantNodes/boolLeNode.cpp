#include "invariantgraph/violationInvariantNodes/boolLeNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::BoolLeNode>
invariantgraph::BoolLeNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2 ||
      constraint.arguments().size() != 3) {
    throw std::runtime_error("BoolLe constraint takes two var bool arguments");
  }
  for (const auto& arg : constraint.arguments()) {
    if (!std::holds_alternative<fznparser::BoolArg>(arg)) {
      throw std::runtime_error(
          "BoolLe constraint takes two var bool arguments");
    }
  }
  VarNodeId a = invariantGraph.createVarNode(
      std::get<fznparser::BoolArg>(constraint.arguments().at(0)));

  VarNodeId b = invariantGraph.createVarNode(
      std::get<fznparser::BoolArg>(constraint.arguments().at(1)));

  if (constraint.arguments().size() == 2) {
    return std::make_unique<BoolLeNode>(a, b, true);
  }

  const fznparser::BoolArg& reified =
      get<fznparser::BoolArg>(constraint.arguments().back());
  if (reified.isFixed()) {
    return std::make_unique<invariantgraph::BoolLeNode>(a, b,
                                                        reified.toParameter());
  }
  return std::make_unique<invariantgraph::BoolLeNode>(
      a, b, invariantGraph.createVarNode(reified.var()));
}

void invariantgraph::BoolLeNode::registerOutputVariables(
    InvariantGraph& invariantGraph, Engine& engine) {
  registerViolation(engine);
}

void invariantgraph::BoolLeNode::registerNode(InvariantGraph& invariantGraph,
                                              Engine& engine) {
  assert(violationVarId() != NULL_ID);
  assert(a()->varId() != NULL_ID);
  assert(b()->varId() != NULL_ID);

  if (shouldHold()) {
    engine.makeConstraint<BoolLessEqual>(engine, violationVarId(), a()->varId(),
                                         b()->varId());
  } else {
    assert(!isReified());
    engine.makeConstraint<BoolLessThan>(engine, violationVarId(), b()->varId(),
                                        a()->varId());
  }
}