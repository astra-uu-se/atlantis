#include "atlantis/invariantgraph/violationInvariantNodes/boolLinLeNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/boolLinear.hpp"
#include "atlantis/propagation/views/greaterEqualConst.hpp"
#include "atlantis/propagation/views/lessEqualConst.hpp"

namespace atlantis::invariantgraph {

BoolLinLeNode::BoolLinLeNode(std::vector<Int>&& coeffs,
                             std::vector<VarNodeId>&& vars, Int bound,
                             VarNodeId reified)
    : ViolationInvariantNode(std::move(vars), reified),
      _coeffs(std::move(coeffs)),
      _bound(bound) {}

BoolLinLeNode::BoolLinLeNode(std::vector<Int>&& coeffs,
                             std::vector<VarNodeId>&& vars, Int bound,
                             bool shouldHold)
    : ViolationInvariantNode(std::move(vars), shouldHold),
      _coeffs(std::move(coeffs)),
      _bound(bound) {}

void BoolLinLeNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  ViolationInvariantNode::init(graph, id);
  assert(!isReified() ||
         !graph.varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::none_of(staticInputVarNodeIds().begin(),
                      staticInputVarNodeIds().end(), [&](const VarNodeId& vId) {
                        return graph.varNodeConst(vId).isIntVar();
                      }));
}

void BoolLinLeNode::updateState(InvariantGraph& graph) {
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

  // Remove fixed inputs and inputs with a coefficient of 0 as well as update
  // _bound:
  std::vector<Int> indicesToRemove;
  indicesToRemove.reserve(staticInputVarNodeIds().size());

  for (Int i = 0; i < static_cast<Int>(staticInputVarNodeIds().size()); ++i) {
    const auto& inputNode = graph.varNodeConst(staticInputVarNodeIds().at(i));
    if (inputNode.isFixed() || _coeffs.at(i) == 0) {
      _bound -= inputNode.inDomain(bool{true}) ? _coeffs.at(i) : 0;
      indicesToRemove.emplace_back(i);
    }
  }

  for (Int i = indicesToRemove.size() - 1; i >= 0; --i) {
    removeStaticInputVarNode(
        graph.varNode(staticInputVarNodeIds().at(indicesToRemove.at(i))));
    _coeffs.erase(_coeffs.begin() + indicesToRemove.at(i));
  }

  // update bounds of output:
  Int lb = _bound;
  Int ub = _bound;
  for (size_t i = 0; i < staticInputVarNodeIds().size(); ++i) {
    lb += std::min<Int>(0, _coeffs.at(i));
    ub += std::max<Int>(0, _coeffs.at(i));
  }

  if (ub <= _bound) {
    if (isReified()) {
      graph.varNode(reifiedViolationNodeId()).fixToValue(bool{true});
      ViolationInvariantNode::updateState(graph);
    }
    if (!shouldHold()) {
      throw InconsistencyException("BoolLinLeNode: Invariant is always false");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (_bound < lb) {
    if (isReified()) {
      graph.varNode(reifiedViolationNodeId()).fixToValue(bool{false});
      ViolationInvariantNode::updateState(graph);
    }
    if (shouldHold()) {
      throw InconsistencyException(
          "BoolLinLeNode neg: Invariant is always false");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
}

void BoolLinLeNode::registerOutputVars(InvariantGraph& graph,
                                       propagation::SolverBase& solver) {
  if (violationVarId(graph) == propagation::NULL_ID) {
    _intermediate = solver.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(graph, solver.makeIntView<propagation::LessEqualConst>(
                                   solver, _intermediate, _bound));
    } else {
      assert(!isReified());
      setViolationVarId(graph,
                        solver.makeIntView<propagation::GreaterEqualConst>(
                            solver, _intermediate, _bound + 1));
    }
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void BoolLinLeNode::registerNode(InvariantGraph& graph,
                                 propagation::SolverBase& solver) {
  assert(violationVarId(graph) != propagation::NULL_ID);

  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const VarNodeId varNodeId) {
                   assert(graph.varId(varNodeId) != propagation::NULL_ID);
                   return graph.varId(varNodeId);
                 });
  solver.makeInvariant<propagation::BoolLinear>(
      solver, _intermediate, std::vector<Int>(_coeffs), std::move(solverVars));
}

}  // namespace atlantis::invariantgraph
