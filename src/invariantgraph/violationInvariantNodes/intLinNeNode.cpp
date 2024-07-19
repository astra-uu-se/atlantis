#include "atlantis/invariantgraph/violationInvariantNodes/intLinNeNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

IntLinNeNode::IntLinNeNode(std::vector<Int>&& coeffs,
                           std::vector<VarNodeId>&& vars, Int bound,
                           VarNodeId reified)
    : ViolationInvariantNode(std::move(vars), reified),
      _coeffs(std::move(coeffs)),
      _bound(bound) {}

IntLinNeNode::IntLinNeNode(std::vector<Int>&& coeffs,
                           std::vector<VarNodeId>&& vars, Int bound,
                           bool shouldHold)
    : ViolationInvariantNode(std::move(vars), shouldHold),
      _coeffs(std::move(coeffs)),
      _bound(bound) {}

void IntLinNeNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  ViolationInvariantNode::init(graph, id);
  assert(!isReified() ||
         !graph.varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::all_of(staticInputVarNodeIds().begin(),
                     staticInputVarNodeIds().end(), [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).isIntVar();
                     }));
}

void IntLinNeNode::updateState(InvariantGraph& graph) {
  ViolationInvariantNode::updateState(graph);
  // Remove duplicates:
  for (Int i = 0; i < static_cast<Int>(staticInputVarNodeIds().size()); ++i) {
    for (Int j = static_cast<Int>(staticInputVarNodeIds().size()) - 1; j > i;
         --j) {
      if (staticInputVarNodeIds().at(i) == staticInputVarNodeIds().at(j)) {
        _coeffs.at(i) += _coeffs.at(j);
        _coeffs.erase(_coeffs.begin() + j);
        eraseStaticInputVarNode(j);
      }
    }
  }

  std::vector<Int> indicesToRemove;
  indicesToRemove.reserve(staticInputVarNodeIds().size());

  for (Int i = 0; i < static_cast<Int>(staticInputVarNodeIds().size()); ++i) {
    const auto& inputNode = graph.varNodeConst(staticInputVarNodeIds().at(i));
    if (inputNode.isFixed() || _coeffs.at(i) == 0) {
      _bound -= _coeffs.at(i) * inputNode.lowerBound();
      indicesToRemove.emplace_back(i);
    }
  }

  for (Int i = indicesToRemove.size() - 1; i >= 0; --i) {
    removeStaticInputVarNode(
        graph.varNode(staticInputVarNodeIds().at(indicesToRemove.at(i))));
    _coeffs.erase(_coeffs.begin() + indicesToRemove.at(i));
  }

  Int lb = 0;
  Int ub = 0;
  for (size_t i = 0; i < staticInputVarNodeIds().size(); ++i) {
    const Int v1 = _coeffs.at(i) *
                   graph.varNode(staticInputVarNodeIds().at(i)).lowerBound();
    const Int v2 = _coeffs.at(i) *
                   graph.varNode(staticInputVarNodeIds().at(i)).upperBound();
    lb += std::min(v1, v2);
    ub += std::max(v1, v2);
  }

  if (_bound < lb || ub < _bound) {
    if (isReified()) {
      graph.varNode(reifiedViolationNodeId()).fixToValue(bool{true});
      ViolationInvariantNode::updateState(graph);
    }
    if (!shouldHold()) {
      throw InconsistencyException(
          "IntLinNeNode neg: Invariant is always false");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (lb == ub && lb == _bound) {
    if (isReified()) {
      graph.varNode(reifiedViolationNodeId()).fixToValue(bool{false});
      ViolationInvariantNode::updateState(graph);
    }
    if (shouldHold()) {
      throw InconsistencyException("IntLinNeNode: Invariant is always false");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
}

void IntLinNeNode::registerOutputVars(InvariantGraph& graph,
                                      propagation::SolverBase& solver) {
  if (violationVarId(graph) == propagation::NULL_ID) {
    _intermediate = solver.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(graph, solver.makeIntView<propagation::NotEqualConst>(
                                   solver, _intermediate, _bound));
    } else {
      assert(!isReified());
      setViolationVarId(graph, solver.makeIntView<propagation::EqualConst>(
                                   solver, _intermediate, _bound));
    }
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void IntLinNeNode::registerNode(InvariantGraph& graph,
                                propagation::SolverBase& solver) {
  assert(violationVarId(graph) != propagation::NULL_ID);

  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const VarNodeId varNodeId) {
                   assert(graph.varId(varNodeId) != propagation::NULL_ID);
                   return graph.varId(varNodeId);
                 });
  solver.makeInvariant<propagation::Linear>(
      solver, _intermediate, std::vector<Int>(_coeffs), std::move(solverVars));
}

const std::vector<Int>& IntLinNeNode::coeffs() const { return _coeffs; }

}  // namespace atlantis::invariantgraph
