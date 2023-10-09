#include "invariantgraph/violationInvariantNodes/boolAndNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

BoolAndNode::BoolAndNode(VarNodeId a, VarNodeId b, VarNodeId r)
    : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}), r) {}
BoolAndNode::BoolAndNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}),
                             shouldHold) {}

std::unique_ptr<BoolAndNode> BoolAndNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2 &&
      constraint.arguments().size() != 3) {
    throw std::runtime_error("BoolAnd constraint takes two var bool arguments");
  }
  for (const auto& arg : constraint.arguments()) {
    if (!std::holds_alternative<fznparser::BoolArg>(arg)) {
      throw std::runtime_error(
          "BoolAnd constraint takes two var bool arguments");
    }
  }
  const auto& a = invariantGraph.createVarNode(
      std::get<fznparser::BoolArg>(constraint.arguments().at(0)));

  const auto& b = invariantGraph.createVarNode(
      std::get<fznparser::BoolArg>(constraint.arguments().at(1)));

  if (constraint.arguments().size() == 2) {
    return std::make_unique<BoolAndNode>(a, b, true);
  }

  const fznparser::BoolArg& reified =
      get<fznparser::BoolArg>(constraint.arguments().back());
  if (reified.isFixed()) {
    return std::make_unique<BoolAndNode>(a, b, reified.toParameter());
  }
  return std::make_unique<BoolAndNode>(
      a, b, invariantGraph.createVarNode(reified.var()));
}

void BoolAndNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                          Engine& engine) {
  if (violationVarId(invariantGraph) == NULL_ID) {
    if (shouldHold()) {
      registerViolation(invariantGraph, engine);
    } else {
      assert(!isReified());
      _intermediate = engine.makeIntVar(0, 0, 0);
      setViolationVarId(invariantGraph, engine.makeIntView<NotEqualConst>(
                                            engine, _intermediate, 0));
    }
  }
}

void BoolAndNode::registerNode(InvariantGraph& invariantGraph, Engine& engine) {
  assert(violationVarId(invariantGraph) != NULL_ID);
  assert(invariantGraph.varId(a()) != NULL_ID);
  assert(invariantGraph.varId(b()) != NULL_ID);

  engine.makeInvariant<BoolAnd>(engine, violationVarId(invariantGraph),
                                invariantGraph.varId(a()),
                                invariantGraph.varId(b()));
}

}  // namespace invariantgraph