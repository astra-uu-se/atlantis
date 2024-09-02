#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/allDifferentNode.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"
#include "atlantis/propagation/violationInvariants/equal.hpp"
#include "atlantis/propagation/violationInvariants/notEqual.hpp"

namespace atlantis::invariantgraph {

IntAllEqualNode::IntAllEqualNode(IInvariantGraph& graph, VarNodeId a,
                                 VarNodeId b, VarNodeId r, bool breaksCycle)
    : IntAllEqualNode(graph, std::vector<VarNodeId>{a, b}, r, breaksCycle) {}

IntAllEqualNode::IntAllEqualNode(IInvariantGraph& graph, VarNodeId a,
                                 VarNodeId b, bool shouldHold, bool breaksCycle)
    : IntAllEqualNode(graph, std::vector<VarNodeId>{a, b}, shouldHold,
                      breaksCycle) {}

IntAllEqualNode::IntAllEqualNode(IInvariantGraph& graph,
                                 std::vector<VarNodeId>&& vars, VarNodeId r,
                                 bool breaksCycle)
    : ViolationInvariantNode(graph, std::move(vars), r),
      _breaksCycle(breaksCycle) {}

IntAllEqualNode::IntAllEqualNode(IInvariantGraph& graph,
                                 std::vector<VarNodeId>&& vars, bool shouldHold,
                                 bool breaksCycle)
    : ViolationInvariantNode(graph, std::move(vars), shouldHold),
      _breaksCycle(breaksCycle) {}

void IntAllEqualNode::init(InvariantNodeId id) {
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

void IntAllEqualNode::updateState() {
  ViolationInvariantNode::updateState();
  if (staticInputVarNodeIds().size() < 2) {
    if (!shouldHold()) {
      throw InconsistencyException(
          "IntAllEqualNode::updateState constraint is violated");
    }
    if (isReified()) {
      fixReified(true);
    }
    setState(InvariantNodeState::SUBSUMED);
  }
  size_t numFixed = 0;
  for (size_t i = 0; i < staticInputVarNodeIds().size(); ++i) {
    const VarNode& iNode =
        invariantGraphConst().varNodeConst(staticInputVarNodeIds().at(i));
    if (!iNode.isFixed()) {
      continue;
    }
    ++numFixed;
    const Int iVal = iNode.lowerBound();
    for (size_t j = i + 1; j < staticInputVarNodeIds().size(); ++j) {
      const VarNode& jNode =
          invariantGraphConst().varNodeConst(staticInputVarNodeIds().at(j));
      if (!jNode.isFixed()) {
        continue;
      }
      const Int jVal = jNode.lowerBound();
      if (iVal != jVal) {
        if (isReified()) {
          fixReified(false);
        } else if (shouldHold()) {
          throw InconsistencyException(
              "IntAllEqualNode::updateState constraint is violated");
        }
      }
    }
  }
  if (numFixed == staticInputVarNodeIds().size()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

void IntAllEqualNode::registerOutputVars() {
  assert(staticInputVarNodeIds().size() >= 2);
  if (violationVarId() == propagation::NULL_ID) {
    if (staticInputVarNodeIds().size() == 2) {
      registerViolation();
    } else if (_allDifferentViolationVarId == propagation::NULL_ID) {
      _allDifferentViolationVarId = solver().makeIntVar(0, 0, 0);
      if (shouldHold()) {
        setViolationVarId(solver().makeIntView<propagation::EqualConst>(
            solver(), _allDifferentViolationVarId,
            staticInputVarNodeIds().size() - 1));
      } else {
        assert(!isReified());
        setViolationVarId(solver().makeIntView<propagation::NotEqualConst>(
            solver(), _allDifferentViolationVarId,
            staticInputVarNodeIds().size() - 1));
      }
    }
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void IntAllEqualNode::registerNode() {
  assert(staticInputVarNodeIds().size() >= 2);

  assert(violationVarId() != propagation::NULL_ID);

  std::vector<propagation::VarViewId> inputVarIds;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(inputVarIds),
                 [&](const auto& id) { return invariantGraph().varId(id); });

  if (inputVarIds.size() == 2) {
    assert(violationVarId().isVar());
    if (shouldHold()) {
      solver().makeViolationInvariant<propagation::Equal>(
          solver(), violationVarId(), inputVarIds.front(), inputVarIds.back());
    } else {
      solver().makeViolationInvariant<propagation::NotEqual>(
          solver(), violationVarId(), inputVarIds.front(), inputVarIds.back());
    }
    return;
  }

  assert(_allDifferentViolationVarId != propagation::NULL_ID);
  assert(_allDifferentViolationVarId.isVar());

  solver().makeViolationInvariant<propagation::AllDifferent>(
      solver(), _allDifferentViolationVarId, std::move(inputVarIds));
}

std::string IntAllEqualNode::dotLangIdentifier() const {
  return "int_all_equal";
}

}  // namespace atlantis::invariantgraph
