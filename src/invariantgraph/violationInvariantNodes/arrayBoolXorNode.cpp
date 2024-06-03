#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolXorNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/boolLinear.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

ArrayBoolXorNode::ArrayBoolXorNode(std::vector<VarNodeId>&& as,
                                   VarNodeId output)
    : ViolationInvariantNode(std::move(as), output) {}

ArrayBoolXorNode::ArrayBoolXorNode(std::vector<VarNodeId>&& as, bool shouldHold)
    : ViolationInvariantNode(std::move(as), shouldHold) {}

void ArrayBoolXorNode::registerOutputVars(InvariantGraph& invariantGraph,
                                          propagation::SolverBase& solver) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    _intermediate = solver.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::EqualConst>(
                            solver, _intermediate, 1));
    } else {
      assert(!isReified());
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::NotEqualConst>(
                            solver, _intermediate, 1));
    }
  }
}

void ArrayBoolXorNode::propagate(InvariantGraph& invariantGraph) {
  ViolationInvariantNode::propagate(invariantGraph);
  std::vector<VarNodeId> fixedInputs;
  fixedInputs.reserve(staticInputVarNodeIds().size());
  VarNodeId trueNodeId = -1;
  size_t numTrue = 0;
  for (const auto& input : staticInputVarNodeIds()) {
    if (invariantGraph.isFixed(input)) {
      fixedInputs.emplace_back(input);
      if (invariantGraph.lowerBound(input) > 0) {
        trueNodeId = input;
        ++numTrue;
      }
    }
  }
  for (const auto& fixedVarNodeId : fixedInputs) {
    removeStaticInputVarNode(invariantGraph.varNode(fixedVarNodeId));
  }
  if (numTrue == 1) {
    if (isReified()) {
      invariantGraph.fixToValue(_reifiedViolationNodeId, true);
      ViolationInvariantNode::propagate(invariantGraph);
    }
    if (shouldHold()) {
      for (const auto& input : staticInputVarNodeIds()) {
        if (input != trueNodeId) {
          invariantGraph.fixToValue(input, false);
        }
      }
      setState(shouldHold() ? InvariantNodeState::SUBSUMED
                            : InvariantNodeState::INFEASIBLE);
    }
  } else if (staticInputVarNodeIds().empty()) {
    if (isReified()) {
      invariantGraph.fixToValue(_reifiedViolationNodeId, false);
      ViolationInvariantNode::propagate(invariantGraph);
    }
    setState(shouldHold() ? InvariantNodeState::INFEASIBLE
                          : InvariantNodeState::SUBSUMED);
  }
}

void ArrayBoolXorNode::registerNode(InvariantGraph& invariantGraph,
                                    propagation::SolverBase& solver) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);
  std::vector<propagation::VarId> inputNodeIds;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(inputNodeIds),
                 [&](const auto& node) { return invariantGraph.varId(node); });

  solver.makeInvariant<propagation::BoolLinear>(solver, _intermediate,
                                                std::move(inputNodeIds));
}

}  // namespace atlantis::invariantgraph
