#include "invariantgraph/violationInvariantNodes/allEqualNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

AllEqualNode::AllEqualNode(std::vector<VarNodeId>&& vars, VarNodeId r)
    : ViolationInvariantNode(std::move(vars), r) {}

AllEqualNode::AllEqualNode(std::vector<VarNodeId>&& vars, bool shouldHold)
    : ViolationInvariantNode(std::move(vars), shouldHold) {}

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

  std::vector<VarNodeId> varNodeIds =
      invariantGraph.createVarNodes(intVarArray);

  if (constraint.arguments().size() == 1) {
    return std::make_unique<AllEqualNode>(std::move(varNodeIds), true);
  }
  const fznparser::BoolArg& reified =
      get<fznparser::BoolArg>(constraint.arguments().back());
  if (reified.isFixed()) {
    return std::make_unique<AllEqualNode>(std::move(varNodeIds),
                                          reified.toParameter());
  }
  return std::make_unique<AllEqualNode>(
      std::move(varNodeIds), invariantGraph.createVarNode(reified.var()));
}

void AllEqualNode::registerOutputVars(InvariantGraph& invariantGraph,
                                      propagation::SolverBase& solver) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    _allDifferentViolationVarId = solver.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::EqualConst>(
                            solver, _allDifferentViolationVarId,
                            staticInputVarNodeIds().size() - 1));
    } else {
      assert(!isReified());
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::NotEqualConst>(
                            solver, _allDifferentViolationVarId,
                            staticInputVarNodeIds().size() - 1));
    }
  }
}

void AllEqualNode::registerNode(InvariantGraph& invariantGraph,
                                propagation::SolverBase& solver) {
  assert(_allDifferentViolationVarId != propagation::NULL_ID);
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  solver.makeViolationInvariant<propagation::AllDifferent>(
      solver, _allDifferentViolationVarId, solverVars);
}

}  // namespace atlantis::invariantgraph