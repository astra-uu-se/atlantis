#include "atlantis/invariantgraph/violationInvariantNodes/allDifferentNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/allDifferentImplicitNode.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"
#include "atlantis/propagation/violationInvariants/notEqual.hpp"

namespace atlantis::invariantgraph {

AllDifferentNode::AllDifferentNode(IInvariantGraph& graph, VarNodeId a,
                                   VarNodeId b, VarNodeId r)
    : AllDifferentNode(graph, std::vector<VarNodeId>{a, b}, r) {}

AllDifferentNode::AllDifferentNode(IInvariantGraph& graph, VarNodeId a,
                                   VarNodeId b, bool shouldHold)
    : AllDifferentNode(graph, std::vector<VarNodeId>{a, b}, shouldHold) {}

AllDifferentNode::AllDifferentNode(IInvariantGraph& graph,
                                   std::vector<VarNodeId>&& vars, VarNodeId r)
    : ViolationInvariantNode(graph, std::move(vars), r) {}

AllDifferentNode::AllDifferentNode(IInvariantGraph& graph,
                                   std::vector<VarNodeId>&& vars,
                                   bool shouldHold)
    : ViolationInvariantNode(graph, std::move(vars), shouldHold) {}

void AllDifferentNode::init(InvariantNodeId id) {
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

void AllDifferentNode::updateState() {
  ViolationInvariantNode::updateState();
  if (isReified() || !shouldHold()) {
    return;
  }
  if (staticInputVarNodeIds().size() <= 1) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool AllDifferentNode::canBeMadeImplicit() const {
  return state() == InvariantNodeState::ACTIVE && !isReified() &&
         shouldHold() &&
         std::all_of(staticInputVarNodeIds().begin(),
                     staticInputVarNodeIds().end(), [&](const auto& id) {
                       return invariantGraphConst()
                           .varNodeConst(id)
                           .definingNodes()
                           .empty();
                     });
}

bool AllDifferentNode::makeImplicit() {
  if (!canBeMadeImplicit()) {
    return false;
  }
  invariantGraph().addImplicitConstraintNode(
      std::make_shared<AllDifferentImplicitNode>(
          invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()}));
  return true;
}

void AllDifferentNode::registerOutputVars() {
  assert(state() == InvariantNodeState::ACTIVE);
  assert(!staticInputVarNodeIds().empty());
  if (!staticInputVarNodeIds().empty() &&
      violationVarId() == propagation::NULL_ID) {
    if (shouldHold()) {
      registerViolation();
    } else {
      assert(!isReified());
      _intermediate = solver().makeIntVar(0, 0, 0);
      setViolationVarId(solver().makeIntView<propagation::NotEqualConst>(
          solver(), _intermediate, 0));
    }
  }
  assert(outputVarNodeIds().size() <= 1);
  assert(outputVarNodeIds().empty() ||
         std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void AllDifferentNode::registerNode() {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  assert(violationVarId() != propagation::NULL_ID);
  assert(shouldHold() || _intermediate != propagation::NULL_ID);
  assert(shouldHold() ? violationVarId().isVar() : _intermediate.isVar());

  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(staticInputVarNodeIds().size());
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& id) { return invariantGraph().varId(id); });

  if (solverVars.size() == 2) {
    solver().makeViolationInvariant<propagation::NotEqual>(
        solver(), !shouldHold() ? _intermediate : violationVarId(),
        solverVars.front(), solverVars.back());
  } else {
    solver().makeViolationInvariant<propagation::AllDifferent>(
        solver(), !shouldHold() ? _intermediate : violationVarId(),
        std::move(solverVars));
  }
}

std::string AllDifferentNode::dotLangIdentifier() const {
  return "all_different";
}

}  // namespace atlantis::invariantgraph
