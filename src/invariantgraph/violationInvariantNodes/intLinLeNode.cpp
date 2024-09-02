#include "atlantis/invariantgraph/violationInvariantNodes/intLinLeNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/views/greaterEqualConst.hpp"
#include "atlantis/propagation/views/lessEqualConst.hpp"

namespace atlantis::invariantgraph {

IntLinLeNode::IntLinLeNode(IInvariantGraph& graph, std::vector<Int>&& coeffs,
                           std::vector<VarNodeId>&& vars, Int bound,
                           VarNodeId reified)
    : ViolationInvariantNode(graph, std::move(vars), reified),
      _coeffs(std::move(coeffs)),
      _bound(bound) {}

IntLinLeNode::IntLinLeNode(IInvariantGraph& graph, std::vector<Int>&& coeffs,
                           std::vector<VarNodeId>&& vars, Int bound,
                           bool shouldHold)
    : ViolationInvariantNode(graph, std::move(vars), shouldHold),
      _coeffs(std::move(coeffs)),
      _bound(bound) {}

void IntLinLeNode::init(InvariantNodeId id) {
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

void IntLinLeNode::updateState() {
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

  if (ub <= _bound) {
    if (isReified()) {
      fixReified(true);
    }
    if (!shouldHold()) {
      throw InconsistencyException("IntLinLeNode: Invariant is always false");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (_bound < lb) {
    if (isReified()) {
      fixReified(false);
    }
    if (shouldHold()) {
      throw InconsistencyException(
          "IntLinLeNode neg: Invariant is always false");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
}

void IntLinLeNode::registerOutputVars() {
  if (violationVarId() == propagation::NULL_ID) {
    _intermediate = solver().makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(solver().makeIntView<propagation::LessEqualConst>(
          solver(), _intermediate, _bound));
    } else {
      assert(!isReified());
      setViolationVarId(solver().makeIntView<propagation::GreaterEqualConst>(
          solver(), _intermediate, _bound + 1));
    }
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void IntLinLeNode::registerNode() {
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

const std::vector<Int>& IntLinLeNode::coeffs() const { return _coeffs; }

std::string IntLinLeNode::dotLangIdentifier() const {
  return "int_lin_le_node";
}

}  // namespace atlantis::invariantgraph
