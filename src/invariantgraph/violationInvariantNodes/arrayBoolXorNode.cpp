#include "invariantgraph/violationInvariantNodes/arrayBoolXorNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

ArrayBoolXorNode::ArrayBoolXorNode(std::vector<VarNodeId>&& as,
                                   VarNodeId output)
    : ViolationInvariantNode(std::move(as), output) {}

ArrayBoolXorNode::ArrayBoolXorNode(std::vector<VarNodeId>&& as, bool shouldHold)
    : ViolationInvariantNode(std::move(as), shouldHold) {}

std::unique_ptr<ArrayBoolXorNode> ArrayBoolXorNode::fromModelConstraint(
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
    return std::make_unique<ArrayBoolXorNode>(std::move(variableNodes),
                                              reified.toParameter());
  }
  return std::make_unique<ArrayBoolXorNode>(
      std::move(variableNodes), invariantGraph.createVarNode(reified.var()));
}

void ArrayBoolXorNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                               propagation::Engine& engine) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    _intermediate = engine.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(invariantGraph, engine.makeIntView<propagation::EqualConst>(
                                            engine, _intermediate, 1));
    } else {
      assert(!isReified());
      setViolationVarId(invariantGraph, engine.makeIntView<propagation::NotEqualConst>(
                                            engine, _intermediate, 1));
    }
  }
}

void ArrayBoolXorNode::registerNode(InvariantGraph& invariantGraph,
                                    propagation::Engine& engine) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);
  std::vector<propagation::VarId> inputs;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(inputs),
                 [&](const auto& node) { return invariantGraph.varId(node); });

  engine.makeInvariant<propagation::BoolLinear>(engine, _intermediate, inputs);
}

}  // namespace invariantgraph