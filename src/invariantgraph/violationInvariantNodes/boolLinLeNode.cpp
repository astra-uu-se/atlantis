#include "invariantgraph/violationInvariantNodes/boolLinLeNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

BoolLinLeNode::BoolLinLeNode(std::vector<Int> coeffs,
                             std::vector<VarNodeId>&& variables, Int bound,
                             VarNodeId r)
    : ViolationInvariantNode(std::move(variables), r),
      _coeffs(std::move(coeffs)),
      _bound(bound) {}

BoolLinLeNode::BoolLinLeNode(std::vector<Int> coeffs,
                             std::vector<VarNodeId>&& variables, Int bound,
                             bool shouldHold)
    : ViolationInvariantNode(std::move(variables), shouldHold),
      _coeffs(std::move(coeffs)),
      _bound(bound) {}

std::unique_ptr<BoolLinLeNode> BoolLinLeNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 3 &&
      constraint.arguments().size() != 4) {
    throw std::runtime_error("BoolLinLe constraint takes two arguments");
  }

  if (!std::holds_alternative<fznparser::IntVarArray>(
          constraint.arguments().at(0))) {
    throw std::runtime_error(
        "BoolLinLe constraint first argument must be an integer array");
  }
  if (!std::holds_alternative<fznparser::BoolVarArray>(
          constraint.arguments().at(1))) {
    throw std::runtime_error(
        "BoolLinLe constraint second argument must be a bool var array");
  }
  if (!std::holds_alternative<fznparser::IntArg>(
          constraint.arguments().at(2))) {
    throw std::runtime_error(
        "BoolLinLe constraint third argument must be an integer");
  }

  const fznparser::IntVarArray& coeffsArg =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0));

  if (!coeffsArg.isParArray()) {
    throw std::runtime_error(
        "BoolLinLe constraint first argument must be an integer array");
  }

  std::vector<int64_t> coeffs = coeffsArg.toParVector();

  std::vector<VarNodeId> boolVarArray = invariantGraph.createVarNodes(
      get<fznparser::BoolVarArray>(constraint.arguments().at(1)));

  const fznparser::IntArg& boundArg =
      std::get<fznparser::IntArg>(constraint.arguments().at(2));

  if (!boundArg.isParameter()) {
    throw std::runtime_error(
        "BoolLinLe constraint third argument must be an integer");
  }

  int64_t bound = boundArg.toParameter();

  if (coeffs.size() != boolVarArray.size()) {
    throw std::runtime_error(
        "BoolLinLe constraint first and second array arguments must have the "
        "same length");
  }

  if (constraint.arguments().size() == 3) {
    return std::make_unique<BoolLinLeNode>(
        std::move(coeffs), std::move(boolVarArray), bound, true);
  }

  const fznparser::BoolArg& reified =
      get<fznparser::BoolArg>(constraint.arguments().back());

  if (reified.isFixed()) {
    return std::make_unique<BoolLinLeNode>(std::move(coeffs),
                                           std::move(boolVarArray), bound,
                                           reified.toParameter());
  }
  return std::make_unique<BoolLinLeNode>(
      std::move(coeffs), std::move(boolVarArray), bound,
      invariantGraph.createVarNode(reified.var()));
}

void BoolLinLeNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                            propagation::Engine& engine) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(invariantGraph, engine.makeIntView<propagation::LessEqualConst>(
                                            engine, _sumVarId, _bound));
    } else {
      assert(!isReified());
      setViolationVarId(invariantGraph, engine.makeIntView<propagation::GreaterEqualConst>(
                                            engine, _sumVarId, _bound + 1));
    }
  }
}

void BoolLinLeNode::registerNode(InvariantGraph& invariantGraph,
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