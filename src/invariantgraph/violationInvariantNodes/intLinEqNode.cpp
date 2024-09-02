#include "atlantis/invariantgraph/violationInvariantNodes/intLinEqNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

IntLinEqNode::IntLinEqNode(IInvariantGraph& graph, std::vector<Int>&& coeffs,
                           std::vector<VarNodeId>&& vars, Int bound,
                           VarNodeId reified)
    : ViolationInvariantNode(graph, std::move(vars), reified),
      _coeffs(std::move(coeffs)),
      _bound(bound) {}

IntLinEqNode::IntLinEqNode(IInvariantGraph& graph, std::vector<Int>&& coeffs,
                           std::vector<VarNodeId>&& vars, Int bound,
                           bool shouldHold)
    : ViolationInvariantNode(graph, std::move(vars), shouldHold),
      _coeffs(std::move(coeffs)),
      _bound(bound) {}

void IntLinEqNode::init(InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(
      std::all_of(staticInputVarNodeIds().begin(),
                  staticInputVarNodeIds().end(), [&](const VarNodeId vId) {
                    return invariantGraphConst().varNodeConst(vId).isIntVar();
                  }));
}

void IntLinEqNode::updateState() {
  ViolationInvariantNode::updateState();
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
    const auto& inputNode =
        invariantGraphConst().varNodeConst(staticInputVarNodeIds().at(i));
    if (inputNode.isFixed() || _coeffs.at(i) == 0) {
      _bound -= _coeffs.at(i) * inputNode.lowerBound();
      indicesToRemove.emplace_back(i);
    }
  }

  for (Int i = indicesToRemove.size() - 1; i >= 0; --i) {
    removeStaticInputVarNode(staticInputVarNodeIds().at(indicesToRemove.at(i)));
    _coeffs.erase(_coeffs.begin() + indicesToRemove.at(i));
  }

  Int lb = 0;
  Int ub = 0;
  for (size_t i = 0; i < staticInputVarNodeIds().size(); ++i) {
    const Int v1 =
        _coeffs.at(i) *
        invariantGraph().varNode(staticInputVarNodeIds().at(i)).lowerBound();
    const Int v2 =
        _coeffs.at(i) *
        invariantGraph().varNode(staticInputVarNodeIds().at(i)).upperBound();
    lb += std::min(v1, v2);
    ub += std::max(v1, v2);
  }

  if (lb == ub && lb == _bound) {
    if (isReified()) {
      fixReified(true);
    }
    if (!shouldHold()) {
      throw InconsistencyException(
          "IntLinEqNode neg: Invariant is always false");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (_bound < lb || ub < _bound) {
    if (isReified()) {
      fixReified(true);
    }
    if (shouldHold()) {
      throw InconsistencyException("IntLinEqNode: Invariant is always false");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
}

void IntLinEqNode::registerOutputVars() {
  if (violationVarId() == propagation::NULL_ID) {
    _intermediate = solver().makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(solver().makeIntView<propagation::EqualConst>(
          solver(), _intermediate, _bound));
    } else {
      assert(!isReified());
      setViolationVarId(solver().makeIntView<propagation::NotEqualConst>(
          solver(), _intermediate, _bound));
    }
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void IntLinEqNode::registerNode() {
  assert(violationVarId() != propagation::NULL_ID);
  assert(violationVarId().isView());

  assert(_intermediate != propagation::NULL_ID);
  assert(_intermediate.isVar());

  std::vector<propagation::VarViewId> solverVars;
  std::transform(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      std::back_inserter(solverVars), [&](const VarNodeId varNodeId) {
        assert(invariantGraph().varId(varNodeId) != propagation::NULL_ID);
        return invariantGraph().varId(varNodeId);
      });
  solver().makeInvariant<propagation::Linear>(solver(), _intermediate,
                                              std::vector<Int>(_coeffs),
                                              std::move(solverVars));
}

const std::vector<Int>& IntLinEqNode::coeffs() const { return _coeffs; }

std::string IntLinEqNode::dotLangIdentifier() const {
  return "int_lin_eq_node";
}

}  // namespace atlantis::invariantgraph
