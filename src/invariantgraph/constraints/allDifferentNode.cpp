#include "invariantgraph/constraints/allDifferentNode.hpp"

#include "../parseHelper.hpp"

invariantgraph::AllDifferentNode::AllDifferentNode(
    std::vector<VariableNode*>&& variables, VariableNode* r)
    : SoftConstraintNode(std::move(variables), r) {}

invariantgraph::AllDifferentNode::AllDifferentNode(
    std::vector<VariableNode*>&& variables, bool shouldHold)
    : SoftConstraintNode(std::move(variables), shouldHold) {}

std::unique_ptr<invariantgraph::AllDifferentNode>
invariantgraph::AllDifferentNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().empty() || constraint.arguments().size() > 2) {
    throw std::runtime_error(
        "AllDifferent constraint takes one or two arguments");
  }
  if (!std::holds_alternative<fznparser::IntVarArray>(
          constraint.arguments().front())) {
    throw std::runtime_error(
        "AllDifferent constraint first argument must be an int var array");
  }
  if (constraint.arguments().size() == 2) {
    if (!std::holds_alternative<fznparser::BoolArg>(
            constraint.arguments().back())) {
      throw std::runtime_error(
          "AllDifferent constraint optional second argument must be a bool "
          "var");
    }
  }
  const auto& intVarArray =
      get<fznparser::IntVarArray>(constraint.arguments().front());
  if (intVarArray.size() == 0 || intVarArray.isParArray()) {
    return nullptr;
  }

  std::vector<VariableNode*> variableNodes =
      pruneAllDifferent(invariantGraph.addVariableArray(intVarArray));

  if (constraint.arguments().size() == 1) {
    return std::make_unique<AllDifferentNode>(std::move(variableNodes), true);
  }

  const fznparser::BoolArg& reified =
      get<fznparser::BoolArg>(constraint.arguments().back());
  if (std::holds_alternative<bool>(reified)) {
    return std::make_unique<invariantgraph::AllDifferentNode>(
        std::move(variableNodes), std::get<bool>(reified));
  }
  return std::make_unique<invariantgraph::AllDifferentNode>(
      std::move(variableNodes),
      invariantGraph.addVariable(
          std::get<std::reference_wrapper<const fznparser::BoolVar>>(reified)
              .get()));
}

void invariantgraph::AllDifferentNode::createDefinedVariables(Engine& engine) {
  if (staticInputs().empty()) {
    return;
  }
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

bool invariantgraph::AllDifferentNode::prune() {
  if (isReified() || !shouldHold()) {
    return false;
  }
  std::vector<VariableNode*> singletonStaticInputs =
      pruneAllDifferent(staticInputs());
  for (auto* const singleton : singletonStaticInputs) {
    removeStaticInput(singleton);
  }
  return !singletonStaticInputs.empty();
}

void invariantgraph::AllDifferentNode::registerWithEngine(Engine& engine) {
  if (staticInputs().empty()) {
    return;
  }
  assert(violationVarId() != NULL_ID);

  std::vector<VarId> engineVariables;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(engineVariables),
                 [&](const auto& var) { return var->varId(); });

  engine.makeConstraint<AllDifferent>(
      engine, !shouldHold() ? _intermediate : violationVarId(),
      engineVariables);
}
