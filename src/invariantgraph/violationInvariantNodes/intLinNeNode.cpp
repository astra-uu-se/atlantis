#include "invariantgraph/violationInvariantNodes/intLinNeNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

IntLinNeNode::IntLinNeNode(std::vector<Int>&& coeffs,
                           std::vector<VarNodeId>&& variables, Int c,
                           VarNodeId r)
    : ViolationInvariantNode(std::move(variables), r),
      _coeffs(std::move(coeffs)),
      _c(c) {}

IntLinNeNode::IntLinNeNode(std::vector<Int>&& coeffs,
                           std::vector<VarNodeId>&& variables, Int c,
                           bool shouldHold)
    : ViolationInvariantNode(std::move(variables), shouldHold),
      _coeffs(std::move(coeffs)),
      _c(c) {}

std::unique_ptr<IntLinNeNode> IntLinNeNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  std::vector<Int> coeffs =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0))
          .toParVector();

  std::vector<VarNodeId> variables = invariantGraph.createVarNodes(
      std::get<fznparser::IntVarArray>(constraint.arguments().at(1)));

  Int bound =
      std::get<Int>(std::get<fznparser::IntArg>(constraint.arguments().at(2)));

  if (constraint.arguments().size() == 4) {
    const fznparser::BoolArg& reified =
        std::get<fznparser::BoolArg>(constraint.arguments().back());

    if (reified.isFixed()) {
      return std::make_unique<IntLinNeNode>(std::move(coeffs),
                                            std::move(variables), bound,
                                            reified.toParameter());
    } else {
      return std::make_unique<IntLinNeNode>(
          std::move(coeffs), std::move(variables), bound,
          invariantGraph.createVarNode(
              std::get<std::reference_wrapper<const fznparser::BoolVar>>(
                  reified)
                  .get()));
    }
  }
  return std::make_unique<IntLinNeNode>(std::move(coeffs), std::move(variables),
                                        bound, true);
}

void IntLinNeNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                           propagation::Engine& engine) {
  if (_sumVarId == propagation::NULL_ID) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(invariantGraph, engine.makeIntView<propagation::NotEqualConst>(
                                            engine, _sumVarId, _c));
    } else {
      assert(!isReified());
      setViolationVarId(invariantGraph,
                        engine.makeIntView<propagation::EqualConst>(engine, _sumVarId, _c));
    }
  }
}

void IntLinNeNode::registerNode(InvariantGraph& invariantGraph,
                                propagation::Engine& engine) {
  std::vector<propagation::VarId> variables;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(variables),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  assert(_sumVarId != propagation::NULL_ID);
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  engine.makeInvariant<propagation::Linear>(engine, _sumVarId, _coeffs, variables);
}

}  // namespace invariantgraph