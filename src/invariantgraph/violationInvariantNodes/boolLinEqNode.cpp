#include "invariantgraph/violationInvariantNodes/boolLinEqNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

BoolLinEqNode::BoolLinEqNode(std::vector<Int>&& coeffs,
                             std::vector<VarNodeId>&& variables, Int c,
                             VarNodeId r)
    : ViolationInvariantNode(std::move(variables), r),
      _coeffs(std::move(coeffs)),
      _c(c) {}

BoolLinEqNode::BoolLinEqNode(std::vector<Int>&& coeffs,
                             std::vector<VarNodeId>&& variables, Int c,
                             bool shouldHold)
    : ViolationInvariantNode(std::move(variables), shouldHold),
      _coeffs(std::move(coeffs)),
      _c(c) {}

std::unique_ptr<BoolLinEqNode> BoolLinEqNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 3 &&
      constraint.arguments().size() != 4) {
    throw std::runtime_error("BoolLinEq constraint takes two arguments");
  }

  if (!std::holds_alternative<fznparser::IntVarArray>(
          constraint.arguments().at(0))) {
    throw std::runtime_error(
        "BoolLinEq constraint first argument must be an integer array");
  }
  if (!std::holds_alternative<fznparser::BoolVarArray>(
          constraint.arguments().at(1))) {
    throw std::runtime_error(
        "BoolLinEq constraint second argument must be a bool var array");
  }
  if (!std::holds_alternative<fznparser::IntArg>(
          constraint.arguments().at(2))) {
    throw std::runtime_error(
        "BoolLinEq constraint third argument must be an integer");
  }

  const fznparser::IntVarArray& coeffsArg =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0));

  if (!coeffsArg.isParArray()) {
    throw std::runtime_error(
        "BoolLinEq constraint first argument must be an integer array");
  }

  std::vector<int64_t> coeffs = coeffsArg.toParVector();

  std::vector<VarNodeId> boolVarArray = invariantGraph.createVarNodes(
      get<fznparser::BoolVarArray>(constraint.arguments().at(1)));

  const fznparser::IntArg& boundArg =
      std::get<fznparser::IntArg>(constraint.arguments().at(2));

  if (!boundArg.isParameter()) {
    throw std::runtime_error(
        "BoolLinEq constraint third argument must be an integer");
  }

  int64_t bound = boundArg.toParameter();

  if (coeffs.size() != boolVarArray.size()) {
    throw std::runtime_error(
        "BoolLinEq constraint first and second array arguments must have the "
        "same length");
  }

  if (constraint.arguments().size() == 3) {
    return std::make_unique<BoolLinEqNode>(
        std::move(coeffs), std::move(boolVarArray), bound, true);
  }

  const fznparser::BoolArg& reified =
      get<fznparser::BoolArg>(constraint.arguments().back());

  if (reified.isFixed()) {
    return std::make_unique<BoolLinEqNode>(std::move(coeffs),
                                           std::move(boolVarArray), bound,
                                           reified.toParameter());
  }
  return std::make_unique<BoolLinEqNode>(
      std::move(coeffs), std::move(boolVarArray), bound,
      invariantGraph.createVarNode(reified.var()));
}

void BoolLinEqNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                            propagation::Engine& engine) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(invariantGraph,
                        engine.makeIntView<propagation::EqualConst>(engine, _sumVarId, _c));
    } else {
      setViolationVarId(invariantGraph, engine.makeIntView<propagation::NotEqualConst>(
                                            engine, _sumVarId, _c));
    }
  }
}

void BoolLinEqNode::registerNode(InvariantGraph& invariantGraph,
                                 propagation::Engine& engine) {
  std::vector<propagation::VarId> variables;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(variables),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  assert(_sumVarId != propagation::NULL_ID);
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  engine.makeInvariant<propagation::BoolLinear>(engine, _sumVarId, _coeffs, variables);
}

}  // namespace invariantgraph