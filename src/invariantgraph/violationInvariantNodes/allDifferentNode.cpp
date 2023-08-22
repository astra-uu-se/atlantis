#include "invariantgraph/violationInvariantNodes/allDifferentNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

AllDifferentNode::AllDifferentNode(std::vector<VarNodeId>&& variables,
                                   VarNodeId r)
    : ViolationInvariantNode(std::move(variables), r) {}

AllDifferentNode::AllDifferentNode(std::vector<VarNodeId>&& variables,
                                   bool shouldHold)
    : ViolationInvariantNode(std::move(variables), shouldHold) {}

std::unique_ptr<AllDifferentNode> AllDifferentNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
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

  std::vector<VarNodeId> variableNodes = pruneAllDifferent(
      invariantGraph, invariantGraph.createVarNodes(intVarArray));

  if (constraint.arguments().size() == 1) {
    return std::make_unique<AllDifferentNode>(std::move(variableNodes), true);
  }

  const fznparser::BoolArg& reified =
      get<fznparser::BoolArg>(constraint.arguments().back());
  if (reified.isFixed()) {
    return std::make_unique<AllDifferentNode>(std::move(variableNodes),
                                              reified.toParameter());
  }
  return std::make_unique<AllDifferentNode>(
      std::move(variableNodes), invariantGraph.createVarNode(reified.var()));
}

void AllDifferentNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                               Engine& engine) {
  if (staticInputVarNodeIds().empty()) {
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

bool AllDifferentNode::prune(InvariantGraph& invariantGraph) {
  if (isReified() || !shouldHold()) {
    return false;
  }
  std::vector<VarNodeId> singletonStaticInputs =
      pruneAllDifferent(invariantGraph, staticInputVarNodeIds());
  for (auto* const singleton : singletonStaticInputs) {
    removeStaticInputVarNode(singleton);
  }
  return !singletonStaticInputs.empty();
}

void AllDifferentNode::registerNode(InvariantGraph& invariantGraph,
                                    Engine& engine) {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  assert(violationVarId() != NULL_ID);

  std::vector<VarId> engineVariables;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(engineVariables),
                 [&](const auto& var) { return var->varId(); });

  engine.makeConstraint<AllDifferent>(
      engine, !shouldHold() ? _intermediate : violationVarId(),
      engineVariables);
}

}  // namespace invariantgraph