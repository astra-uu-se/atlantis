#include "invariantgraph/constraints/boolLinLeNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<BoolLinLeNode> BoolLinLeNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 3 ||
      constraint.arguments().size() != 4) {
    throw std::runtime_error("BoolLinLe constraint takes two arguments");
  }

  if (!std::holds_alternative<fznparser::IntVarArray>(
          constraint.arguments().at(0))) {
    throw std::runtime_error(
        "BoolLinLe constraint first argument must be an integer array");
  }
  if (!std::holds_alternative<fznparser::BoolArg>(
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

  std::vector<VariableNode*> boolVarArray = invariantGraph.addVariableArray(
      get<fznparser::IntVarArray>(constraint.arguments().front()));

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

  if (std::holds_alternative<bool>(reified)) {
    return std::make_unique<BoolLinLeNode>(std::move(coeffs),
                                           std::move(boolVarArray), bound,
                                           std::get<bool>(reified));
  }
  return std::make_unique<BoolLinLeNode>(
      std::move(coeffs), std::move(boolVarArray), bound,
      invariantGraph.addVariable(
          std::get<std::reference_wrapper<const fznparser::BoolVar>>(reified)
              .get()));
}

void BoolLinLeNode::createDefinedVariables(Engine& engine) {
  if (violationVarId() == NULL_ID) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(
          engine.makeIntView<LessEqualConst>(engine, _sumVarId, _bound));
    } else {
      assert(!isReified());
      setViolationVarId(
          engine.makeIntView<GreaterEqualConst>(engine, _sumVarId, _bound + 1));
    }
  }
}

void BoolLinLeNode::registerWithEngine(Engine& engine) {
  std::vector<VarId> variables;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(variables),
                 [&](auto node) { return node->varId(); });

  assert(_sumVarId != NULL_ID);
  assert(violationVarId() != NULL_ID);

  engine.makeInvariant<BoolLinear>(engine, _sumVarId, _coeffs, variables);
}

}  // namespace invariantgraph