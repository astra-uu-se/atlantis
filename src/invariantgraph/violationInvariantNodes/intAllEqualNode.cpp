#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intEqNode.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"
#include "atlantis/propagation/violationInvariants/equal.hpp"

namespace atlantis::invariantgraph {

IntAllEqualNode::IntAllEqualNode(std::vector<VarNodeId>&& vars, VarNodeId r,
                                 bool breaksCycle)
    : ViolationInvariantNode(std::move(vars), r), _breaksCycle(breaksCycle) {}

IntAllEqualNode::IntAllEqualNode(std::vector<VarNodeId>&& vars, bool shouldHold,
                                 bool breaksCycle)
    : ViolationInvariantNode(std::move(vars), shouldHold),
      _breaksCycle(breaksCycle) {}

bool IntAllEqualNode::canBeReplaced(const InvariantGraph&) const {
  if (staticInputVarNodeIds().size() <= 2) {
    return true;
  }
  return false;
}

bool IntAllEqualNode::replace(InvariantGraph& invariantGraph) {
  if (!canBeReplaced(invariantGraph)) {
    return false;
  }
  if (staticInputVarNodeIds().size() <= 1) {
    deactivate(invariantGraph);
    return true;
  }
  if (staticInputVarNodeIds().size() == 2) {
    if (isReified()) {
      invariantGraph.addInvariantNode(std::make_unique<IntEqNode>(
          staticInputVarNodeIds().front(), staticInputVarNodeIds().back(),
          outputVarNodeIds().front(), _breaksCycle));
    } else {
      invariantGraph.addInvariantNode(std::make_unique<IntEqNode>(
          staticInputVarNodeIds().front(), staticInputVarNodeIds().back(),
          shouldHold(), _breaksCycle));
    }
  }

  return false;
}

void IntAllEqualNode::registerOutputVars(InvariantGraph& invariantGraph,
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

void IntAllEqualNode::registerNode(InvariantGraph& invariantGraph,
                                   propagation::SolverBase& solver) {
  assert(_allDifferentViolationVarId != propagation::NULL_ID);
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  std::vector<propagation::VarId> inputVarIds;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(inputVarIds),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  solver.makeViolationInvariant<propagation::AllDifferent>(
      solver, _allDifferentViolationVarId, std::move(inputVarIds));
}

}  // namespace atlantis::invariantgraph
