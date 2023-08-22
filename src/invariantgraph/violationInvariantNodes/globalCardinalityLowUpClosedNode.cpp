#include "invariantgraph/violationInvariantNodes/globalCardinalityLowUpClosedNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<GlobalCardinalityLowUpClosedNode>
GlobalCardinalityLowUpClosedNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  std::vector<VarNodeId> inputs = invariantGraph.createVarNodes(
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0)));

  std::vector<Int> cover =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(1))
          .toParVector();

  std::vector<Int> low =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(2))
          .toParVector();

  std::vector<Int> up =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(3))
          .toParVector();

  assert(cover.size() == low.size());
  assert(cover.size() == up.size());

  bool shouldHold = true;
  VarNodeId r = nullptr;

  if (constraint.arguments().size() == 5) {
    const fznparser::BoolArg reified =
        std::get<fznparser::BoolArg>(constraint.arguments().at(4));
    if (reified.isFixed()) {
      shouldHold = reified.toParameter();
    } else {
      r = invariantGraph.createVarNode(reified.var());
    }
  }

  if (r != nullptr) {
    return std::make_unique<GlobalCardinalityLowUpClosedNode>(
        std::move(inputs), std::move(cover), std::move(low), std::move(up), r);
  }
  assert(r == nullptr);
  return std::make_unique<GlobalCardinalityLowUpClosedNode>(
      std::move(inputs), std::move(cover), std::move(low), std::move(up),
      shouldHold);
}

void GlobalCardinalityLowUpClosedNode::registerOutputVariables(
    InvariantGraph& invariantGraph, Engine& engine) {
  if (violationVarId() == NULL_ID) {
    if (!shouldHold()) {
      _intermediate = engine.makeIntVar(0, 0, staticInputVarNodeIds().size());
      setViolationVarId(
          engine.makeIntView<NotEqualConst>(engine, _intermediate, 0));
    } else {
      registerViolation(engine);
    }
  }
}

void GlobalCardinalityLowUpClosedNode::registerNode(
    InvariantGraph& invariantGraph, Engine& engine) {
  std::vector<VarId> inputs;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(inputs),
                 [&](auto node) { return node->varId(); });

  if (shouldHold()) {
    engine.makeInvariant<GlobalCardinalityConst<true>>(
        engine, violationVarId(), inputs, _cover, _low, _up);
  } else {
    engine.makeInvariant<GlobalCardinalityConst<true>>(
        engine, _intermediate, inputs, _cover, _low, _up);
  }
}

}  // namespace invariantgraph