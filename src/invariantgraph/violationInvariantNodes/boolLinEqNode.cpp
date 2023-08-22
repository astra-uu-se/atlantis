#include "invariantgraph/violationInvariantNodes/boolLinEqNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::BoolLinEqNode>
invariantgraph::BoolLinEqNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 3 ||
      constraint.arguments().size() != 4) {
    throw std::runtime_error("BoolLinEq constraint takes two arguments");
  }

  if (!std::holds_alternative<fznparser::IntVarArray>(
          constraint.arguments().at(0))) {
    throw std::runtime_error(
        "BoolLinEq constraint first argument must be an integer array");
  }
  if (!std::holds_alternative<fznparser::BoolArg>(
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
      get<fznparser::IntVarArray>(constraint.arguments().front()));

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

void invariantgraph::BoolLinEqNode::registerOutputVariables(
    InvariantGraph& invariantGraph, Engine& engine) {
  if (violationVarId() == NULL_ID) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(engine.makeIntView<EqualConst>(engine, _sumVarId, _c));
    } else {
      setViolationVarId(
          engine.makeIntView<NotEqualConst>(engine, _sumVarId, _c));
    }
  }
}

void invariantgraph::BoolLinEqNode::registerNode(InvariantGraph& invariantGraph,
                                                 Engine& engine) {
  std::vector<VarId> variables;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(variables),
                 [&](auto node) { return node->varId(); });

  assert(_sumVarId != NULL_ID);
  assert(violationVarId() != NULL_ID);

  engine.makeInvariant<BoolLinear>(engine, _sumVarId, _coeffs, variables);
}
