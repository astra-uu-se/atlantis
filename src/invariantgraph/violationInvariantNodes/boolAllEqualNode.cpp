#include "atlantis/invariantgraph/violationInvariantNodes/boolAllEqualNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/boolAllEqual.hpp"

namespace atlantis::invariantgraph {

BoolAllEqualNode::BoolAllEqualNode(std::vector<VarNodeId>&& vars, VarNodeId r,
                                   bool breaksCycle)
    : ViolationInvariantNode(std::move(vars), r), _breaksCycle(breaksCycle) {}

BoolAllEqualNode::BoolAllEqualNode(std::vector<VarNodeId>&& vars,
                                   bool shouldHold, bool breaksCycle)
    : ViolationInvariantNode(std::move(vars), shouldHold),
      _breaksCycle(breaksCycle) {}

void BoolAllEqualNode::registerOutputVars(InvariantGraph& invariantGraph,
                                          propagation::SolverBase& solver) {
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

void BoolAllEqualNode::registerNode(InvariantGraph& invariantGraph,
                                    propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  solver.makeViolationInvariant<propagation::BoolAllEqual>(
      solver, !shouldHold() ? _intermediate : violationVarId(invariantGraph),
      std::move(solverVars));
}

}  // namespace atlantis::invariantgraph
