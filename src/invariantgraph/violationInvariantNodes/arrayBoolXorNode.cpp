#include "invariantgraph/violationInvariantNodes/arrayBoolXorNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::ArrayBoolXorNode>
invariantgraph::ArrayBoolXorNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2) {
    throw std::runtime_error("ArrayBoolXOr constraint takes two arguments");
  }
  if (!std::holds_alternative<fznparser::IntVarArray>(
          constraint.arguments().front())) {
    throw std::runtime_error(
        "ArrayBoolXOr constraint first argument must be a bool var array");
  }
  if (!std::holds_alternative<fznparser::BoolArg>(
          constraint.arguments().back())) {
    throw std::runtime_error(
        "ArrayBoolXOr constraint optional second argument must be a bool "
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
    return std::make_unique<invariantgraph::ArrayBoolXorNode>(
        std::move(variableNodes), reified.toParameter());
  }
  return std::make_unique<invariantgraph::ArrayBoolXorNode>(
      std::move(variableNodes), invariantGraph.createVarNode(reified.var()));
}

void invariantgraph::ArrayBoolXorNode::registerOutputVariables(
    InvariantGraph& invariantGraph, Engine& engine) {
  if (violationVarId() == NULL_ID) {
    _intermediate = engine.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(
          engine.makeIntView<EqualConst>(engine, _intermediate, 1));
    } else {
      assert(!isReified());
      setViolationVarId(
          engine.makeIntView<NotEqualConst>(engine, _intermediate, 1));
    }
  }
}

void invariantgraph::ArrayBoolXorNode::registerNode(
    InvariantGraph& invariantGraph, Engine& engine) {
  assert(violationVarId() != NULL_ID);
  std::vector<VarId> inputs;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(inputs),
                 [&](const auto& node) { return node->varId(); });

  engine.makeInvariant<BoolLinear>(engine, _intermediate, inputs);
}
