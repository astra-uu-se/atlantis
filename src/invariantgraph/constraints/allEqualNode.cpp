#include "invariantgraph/constraints/allEqualNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::AllEqualNode>
invariantgraph::AllEqualNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    InvariantGraph& invariantGraph) {
  if (constraint.arguments().empty() || constraint.arguments().size() > 2) {
    throw std::runtime_error("AllEqual constraint takes one or two arguments");
  }
  if (!std::holds_alternative<fznparser::IntVarArray>(
          constraint.arguments().front())) {
    throw std::runtime_error(
        "AllEqual constraint first argument must be an int var array");
  }
  if (constraint.arguments().size() == 2) {
    if (!std::holds_alternative<fznparser::BoolArg>(
            constraint.arguments().back())) {
      throw std::runtime_error(
          "AllEqual constraint optional second argument must be a bool "
          "var");
    }
  }
  const auto& intVarArray =
      get<fznparser::IntVarArray>(constraint.arguments().front());
  if (intVarArray.size() == 0 || intVarArray.isParArray()) {
    return nullptr;
  }

  std::vector<VariableNode*> variableNodes =
      invariantGraph.addVariableArray(intVarArray);

  if (constraint.arguments().size() == 1) {
    return std::make_unique<AllEqualNode>(std::move(variableNodes), true);
  }
  const fznparser::BoolArg& reified =
      get<fznparser::BoolArg>(constraint.arguments().back());
  if (std::holds_alternative<bool>(reified)) {
    return std::make_unique<invariantgraph::AllEqualNode>(
        std::move(variableNodes), std::get<bool>(reified));
  }
  return std::make_unique<invariantgraph::AllEqualNode>(
      std::move(variableNodes),
      invariantGraph.addVariable(
          std::get<std::reference_wrapper<const fznparser::BoolVar>>(reified)
              .get()));
}

void invariantgraph::AllEqualNode::createDefinedVariables(Engine& engine) {
  if (violationVarId() == NULL_ID) {
    _allDifferentViolationVarId = engine.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(engine.makeIntView<EqualConst>(
          engine, _allDifferentViolationVarId, staticInputs().size() - 1));
    } else {
      assert(!isReified());
      setViolationVarId(engine.makeIntView<NotEqualConst>(
          engine, _allDifferentViolationVarId, staticInputs().size() - 1));
    }
  }
}

void invariantgraph::AllEqualNode::registerWithEngine(Engine& engine) {
  assert(_allDifferentViolationVarId != NULL_ID);
  assert(violationVarId() != NULL_ID);

  std::vector<VarId> engineVariables;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(engineVariables),
                 [&](const auto& var) { return var->varId(); });

  engine.makeConstraint<AllDifferent>(engine, _allDifferentViolationVarId,
                                      engineVariables);
}
