#include "invariantgraph/violationInvariantNodes/arrayBoolOrNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

ArrayBoolOrNode::ArrayBoolOrNode(std::vector<VarNodeId>&& as, VarNodeId output)
    : ViolationInvariantNode(std::move(as), output) {}

ArrayBoolOrNode::ArrayBoolOrNode(std::vector<VarNodeId>&& as, bool shouldHold)
    : ViolationInvariantNode(std::move(as), shouldHold) {}

std::unique_ptr<ArrayBoolOrNode> ArrayBoolOrNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2) {
    throw std::runtime_error("ArrayBoolOr constraint takes two arguments");
  }
  if (!std::holds_alternative<fznparser::BoolVarArray>(
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
      get<fznparser::BoolVarArray>(constraint.arguments().front());

  if (boolVarArray.size() == 0 || boolVarArray.isParArray()) {
    return nullptr;
  }

  std::vector<VarNodeId> variableNodes =
      invariantGraph.createVarNodes(boolVarArray);

  const fznparser::BoolArg& reified =
      get<fznparser::BoolArg>(constraint.arguments().back());
  if (reified.isFixed()) {
    return std::make_unique<ArrayBoolOrNode>(std::move(variableNodes),
                                             reified.toParameter());
  }
  return std::make_unique<ArrayBoolOrNode>(
      std::move(variableNodes), invariantGraph.createVarNode(reified.var()));
}

void ArrayBoolOrNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                              Engine& engine) {
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

void ArrayBoolOrNode::registerNode(InvariantGraph& invariantGraph,
                                   Engine& engine) {
  assert(violationVarId(invariantGraph) != NULL_ID);
  std::vector<VarId> inputs;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(inputs),
                 [&](const auto& node) { return invariantGraph.varId(node); });

  engine.makeInvariant<Exists>(
      engine, !shouldHold() ? _intermediate : violationVarId(invariantGraph),
      inputs);
}

}  // namespace invariantgraph