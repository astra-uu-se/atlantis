#include "invariantgraph/violationInvariantNodes/allDifferentNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

AllDifferentNode::AllDifferentNode(std::vector<VarNodeId>&& vars, VarNodeId r)
    : ViolationInvariantNode(std::move(vars), r) {}

AllDifferentNode::AllDifferentNode(std::vector<VarNodeId>&& vars,
                                   bool shouldHold)
    : ViolationInvariantNode(std::move(vars), shouldHold) {}

std::unique_ptr<AllDifferentNode> AllDifferentNode::fromModelConstraint(
    const fznparser::Constraint& constraint, FznInvariantGraph& invariantGraph) {
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

  std::vector<VarNodeId> varNodeIds = pruneAllDifferentFree(
      invariantGraph, invariantGraph.createVarNodes(intVarArray));

  if (constraint.arguments().size() == 1) {
    return std::make_unique<AllDifferentNode>(std::move(varNodeIds), true);
  }

  const fznparser::BoolArg& reified =
      get<fznparser::BoolArg>(constraint.arguments().back());
  if (reified.isFixed()) {
    return std::make_unique<AllDifferentNode>(std::move(varNodeIds),
                                              reified.toParameter());
  }
  return std::make_unique<AllDifferentNode>(
      std::move(varNodeIds), invariantGraph.createVarNode(reified.var()));
}

void AllDifferentNode::registerOutputVars(InvariantGraph& invariantGraph,
                                          propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    if (shouldHold()) {
      registerViolation(invariantGraph, solver);
    } else {
      assert(!isReified());
      _intermediate = solver.makeIntVar(0, 0, 0);
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::NotEqualConst>(
                            solver, _intermediate, 0));
    }
  }
}

bool AllDifferentNode::prune(InvariantGraph& invariantGraph) {
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

void AllDifferentNode::registerNode(InvariantGraph& invariantGraph,
                                    propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  solver.makeViolationInvariant<propagation::AllDifferent>(
      solver, !shouldHold() ? _intermediate : violationVarId(invariantGraph),
      solverVars);
}

}  // namespace atlantis::invariantgraph