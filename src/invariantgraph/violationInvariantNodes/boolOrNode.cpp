#include "invariantgraph/violationInvariantNodes/boolOrNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<BoolOrNode> BoolOrNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2 ||
      constraint.arguments().size() != 3) {
    throw std::runtime_error("BoolOr constraint takes two var bool arguments");
  }
  for (const auto& arg : constraint.arguments()) {
    if (!std::holds_alternative<fznparser::BoolArg>(arg)) {
      throw std::runtime_error(
          "BoolOr constraint takes two var bool arguments");
    }
  }

  VarNodeId a = invariantGraph.createVarNode(
      std::get<fznparser::BoolArg>(constraint.arguments().at(0)));

  VarNodeId b = invariantGraph.createVarNode(
      std::get<fznparser::BoolArg>(constraint.arguments().at(1)));

  if (constraint.arguments().size() == 2) {
    return std::make_unique<BoolOrNode>(a, b, true);
  }

  const auto& reified = get<fznparser::BoolArg>(constraint.arguments().at(2));
  if (reified.isFixed()) {
    return std::make_unique<BoolOrNode>(a, b, reified.toParameter());
  }
  return std::make_unique<BoolOrNode>(
      a, b, invariantGraph.createVarNode(reified.var()));
}

void BoolOrNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                         Engine& engine) {
  if (violationVarId() == NULL_ID) {
    if (shouldHold()) {
      registerViolation(engine);
    } else {
      assert(!isReified());
      _intermediate = engine.makeIntVar(0, 0, 0);
      setViolationVarId(
          engine.makeIntView<NotEqualConst>(engine, _intermediate, 0));
    }
  }
}

void BoolOrNode::registerNode(InvariantGraph& invariantGraph, Engine& engine) {
  assert(violationVarId() != NULL_ID);

  engine.makeInvariant<BoolOr>(engine, violationVarId(), a()->varId(),
                               b()->varId());
}

}  // namespace invariantgraph