#include "invariantgraph/constraints/arrayBoolAndNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<ArrayBoolAndNode> ArrayBoolAndNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));
  if (constraint.arguments().size() != 2) {
    throw std::runtime_error("ArrayBoolAnd constraint takes two arguments");
  }
  if (!std::holds_alternative<fznparser::IntVarArray>(
          constraint.arguments().front())) {
    throw std::runtime_error(
        "ArrayBoolAnd constraint first argument must be an bool var array");
  }
  if (!std::holds_alternative<fznparser::BoolArg>(
          constraint.arguments().back())) {
    throw std::runtime_error(
        "ArrayBoolAnd constraint optional second argument must be a bool "
        "var");
  }
  const auto& boolVarArray =
      get<fznparser::IntVarArray>(constraint.arguments().front());

  if (boolVarArray.size() == 0 || boolVarArray.isParArray()) {
    return nullptr;
  }

  std::vector<VariableNode*> variableNodes =
      invariantGraph.addVariableArray(boolVarArray);

  const fznparser::BoolArg& reified =
      get<fznparser::BoolArg>(constraint.arguments().back());

  if (std::holds_alternative<bool>(reified)) {
    return std::make_unique<ArrayBoolAndNode>(std::move(variableNodes),
                                              std::get<bool>(reified));
  }
  return std::make_unique<ArrayBoolAndNode>(
      std::move(variableNodes),
      invariantGraph.addVariable(
          std::get<std::reference_wrapper<const fznparser::BoolVar>>(reified)
              .get()));
}

void ArrayBoolAndNode::createDefinedVariables(Engine& engine) {
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

void ArrayBoolAndNode::registerWithEngine(Engine& engine) {
  assert(violationVarId() != NULL_ID);
  std::vector<VarId> inputs;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(inputs),
                 [&](const auto& node) { return node->varId(); });

  engine.makeInvariant<ForAll>(
      engine, !shouldHold() ? _intermediate : violationVarId(), inputs);
}

}  // namespace invariantgraph