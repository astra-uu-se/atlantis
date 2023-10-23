#include "invariantgraph/violationInvariantNodes/boolOrNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

BoolOrNode::BoolOrNode(VarNodeId a, VarNodeId b, VarNodeId r)
    : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}), r) {}
BoolOrNode::BoolOrNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}),
                             shouldHold) {}

std::unique_ptr<BoolOrNode> BoolOrNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2 &&
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
                                         propagation::Engine& engine) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    if (shouldHold()) {
      registerViolation(invariantGraph, engine);
    } else {
      assert(!isReified());
      _intermediate = engine.makeIntVar(0, 0, 0);
      setViolationVarId(invariantGraph, engine.makeIntView<propagation::NotEqualConst>(
                                            engine, _intermediate, 0));
    }
  }
}

void BoolOrNode::registerNode(InvariantGraph& invariantGraph, propagation::Engine& engine) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  engine.makeInvariant<propagation::BoolOr>(engine, violationVarId(invariantGraph),
                               invariantGraph.varId(a()),
                               invariantGraph.varId(b()));
}

}  // namespace invariantgraph