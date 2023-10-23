#include "invariantgraph/violationInvariantNodes/boolLeNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

BoolLeNode::BoolLeNode(VarNodeId a, VarNodeId b, VarNodeId r)
    : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}), r) {}

BoolLeNode::BoolLeNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}),
                             shouldHold) {}

std::unique_ptr<BoolLeNode> BoolLeNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2 &&
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
    return std::make_unique<BoolLeNode>(a, b, reified.toParameter());
  }
  return std::make_unique<BoolLeNode>(
      a, b, invariantGraph.createVarNode(reified.var()));
}

void BoolLeNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                         propagation::Engine& engine) {
  registerViolation(invariantGraph, engine);
}

void BoolLeNode::registerNode(InvariantGraph& invariantGraph, propagation::Engine& engine) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);
  assert(invariantGraph.varId(a()) != propagation::NULL_ID);
  assert(invariantGraph.varId(b()) != propagation::NULL_ID);

  if (shouldHold()) {
    engine.makeConstraint<propagation::BoolLessEqual>(engine, violationVarId(invariantGraph),
                                         invariantGraph.varId(a()),
                                         invariantGraph.varId(b()));
  } else {
    assert(!isReified());
    engine.makeConstraint<propagation::BoolLessThan>(engine, violationVarId(invariantGraph),
                                        invariantGraph.varId(b()),
                                        invariantGraph.varId(a()));
  }
}

}  // namespace invariantgraph