#include "invariantgraph/violationInvariantNodes/allEqualNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

AllEqualNode::AllEqualNode(std::vector<VarNodeId>&& variables, VarNodeId r)
    : ViolationInvariantNode(std::move(variables), r) {}

AllEqualNode::AllEqualNode(std::vector<VarNodeId>&& variables, bool shouldHold)
    : ViolationInvariantNode(std::move(variables), shouldHold) {}

std::unique_ptr<AllEqualNode> AllEqualNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
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

  std::vector<VarNodeId> variableNodes =
      invariantGraph.createVarNodes(intVarArray);

  if (constraint.arguments().size() == 1) {
    return std::make_unique<AllEqualNode>(std::move(variableNodes), true);
  }
  const fznparser::BoolArg& reified =
      get<fznparser::BoolArg>(constraint.arguments().back());
  if (reified.isFixed()) {
    return std::make_unique<AllEqualNode>(std::move(variableNodes),
                                          reified.toParameter());
  }
  return std::make_unique<AllEqualNode>(
      std::move(variableNodes), invariantGraph.createVarNode(reified.var()));
}

void AllEqualNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                           Engine& engine) {
  if (violationVarId(invariantGraph) == NULL_ID) {
    _allDifferentViolationVarId = engine.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(
          invariantGraph,
          engine.makeIntView<EqualConst>(engine, _allDifferentViolationVarId,
                                         staticInputVarNodeIds().size() - 1));
    } else {
      assert(!isReified());
      setViolationVarId(invariantGraph,
                        engine.makeIntView<NotEqualConst>(
                            engine, _allDifferentViolationVarId,
                            staticInputVarNodeIds().size() - 1));
    }
  }
}

void AllEqualNode::registerNode(InvariantGraph& invariantGraph,
                                Engine& engine) {
  assert(_allDifferentViolationVarId != NULL_ID);
  assert(violationVarId(invariantGraph) != NULL_ID);

  std::vector<VarId> engineVariables;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(engineVariables),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  engine.makeConstraint<AllDifferent>(engine, _allDifferentViolationVarId,
                                      engineVariables);
}

}  // namespace invariantgraph