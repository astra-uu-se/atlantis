#include "invariantgraph/violationInvariantNodes/arrayBoolOrNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::ArrayBoolOrNode>
invariantgraph::ArrayBoolOrNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2) {
    throw std::runtime_error("ArrayBoolOr constraint takes two arguments");
  }
  if (!std::holds_alternative<fznparser::IntVarArray>(
          constraint.arguments().front())) {
    throw std::runtime_error(
        "ArrayBoolOr constraint first argument must be a bool var array");
  }
  if (!std::holds_alternative<fznparser::BoolArg>(
          constraint.arguments().back())) {
    throw std::runtime_error(
        "ArrayBoolOr constraint optional second argument must be a bool "
        "var");
  }
  const auto& boolVarArray =
      get<fznparser::IntVarArray>(constraint.arguments().front());

  if (boolVarArray.size() == 0 || boolVarArray.isParArray()) {
    return nullptr;
  }

  std::vector<VarNodeId> variableNodes =
      invariantGraph.createVarNodes(boolVarArray);

  const fznparser::BoolArg& reified =
      get<fznparser::BoolArg>(constraint.arguments().back());
  if (reified.isFixed()) {
    return std::make_unique<invariantgraph::ArrayBoolOrNode>(
        std::move(variableNodes), reified.toParameter());
  }
  return std::make_unique<invariantgraph::ArrayBoolOrNode>(
      std::move(variableNodes), invariantGraph.createVarNode(reified.var()));
}

void invariantgraph::ArrayBoolOrNode::registerOutputVariables(
    InvariantGraph& invariantGraph, Engine& engine) {
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

void invariantgraph::ArrayBoolOrNode::registerNode(
    InvariantGraph& invariantGraph, Engine& engine) {
  assert(violationVarId() != NULL_ID);
  std::vector<VarId> inputs;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(inputs),
                 [&](const auto& node) { return node->varId(); });

  engine.makeInvariant<Exists>(
      engine, !shouldHold() ? _intermediate : violationVarId(), inputs);
}
