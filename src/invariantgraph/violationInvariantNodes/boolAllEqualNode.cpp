#include "invariantgraph/violationInvariantNodes/boolAllEqualNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

BoolAllEqualNode::BoolAllEqualNode(std::vector<VarNodeId>&& variables,
                                   VarNodeId r)
    : ViolationInvariantNode(std::move(variables), r) {}

BoolAllEqualNode::BoolAllEqualNode(std::vector<VarNodeId>&& variables,
                                   bool shouldHold)
    : ViolationInvariantNode(std::move(variables), shouldHold) {}

std::unique_ptr<BoolAllEqualNode> BoolAllEqualNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().empty() || constraint.arguments().size() > 2) {
    throw std::runtime_error(
        "BoolAllEqual constraint takes one or two arguments");
  }
  if (!std::holds_alternative<fznparser::BoolVarArray>(
          constraint.arguments().front())) {
    throw std::runtime_error(
        "BoolAllEqual constraint first argument must be an bool var array");
  }
  if (constraint.arguments().size() == 2) {
    if (!std::holds_alternative<fznparser::BoolArg>(
            constraint.arguments().back())) {
      throw std::runtime_error(
          "BoolAllEqual constraint optional second argument must be a bool "
          "var");
    }
  }
  const auto& intVarArray =
      get<fznparser::BoolVarArray>(constraint.arguments().front());

  if (intVarArray.size() == 0 || intVarArray.isParArray()) {
    return nullptr;
  }

  std::vector<VarNodeId> variableNodes = pruneAllDifferentFree(
      invariantGraph, invariantGraph.createVarNodes(intVarArray));

  if (constraint.arguments().size() == 1) {
    return std::make_unique<BoolAllEqualNode>(std::move(variableNodes), true);
  }

  const fznparser::BoolArg& reified =
      get<fznparser::BoolArg>(constraint.arguments().back());
  if (reified.isFixed()) {
    return std::make_unique<BoolAllEqualNode>(std::move(variableNodes),
                                              reified.toParameter());
  }
  return std::make_unique<BoolAllEqualNode>(
      std::move(variableNodes), invariantGraph.createVarNode(reified.var()));
}

void BoolAllEqualNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                               Engine& engine) {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  if (violationVarId(invariantGraph) == NULL_ID) {
    if (shouldHold()) {
      registerViolation(invariantGraph, engine);
    } else {
      assert(!isReified());
      _intermediate = engine.makeIntVar(0, 0, 0);
      setViolationVarId(invariantGraph, engine.makeIntView<NotEqualConst>(
                                            engine, _intermediate, 0));
    }
  }
}

bool BoolAllEqualNode::prune(InvariantGraph& invariantGraph) {
  if (isReified() || !shouldHold()) {
    return false;
  }
  std::vector<VarNodeId> fixedInputs =
      pruneAllDifferentFixed(invariantGraph, staticInputVarNodeIds());

  for (const auto& fixedVarNodeId : fixedInputs) {
    removeStaticInputVarNode(invariantGraph.varNode(fixedVarNodeId));
  }

  return !fixedInputs.empty();
}

void BoolAllEqualNode::registerNode(InvariantGraph& invariantGraph,
                                    Engine& engine) {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  assert(violationVarId(invariantGraph) != NULL_ID);

  std::vector<VarId> engineVariables;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(engineVariables),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  engine.makeConstraint<BoolAllEqual>(
      engine, !shouldHold() ? _intermediate : violationVarId(invariantGraph),
      engineVariables);
}

}  // namespace invariantgraph