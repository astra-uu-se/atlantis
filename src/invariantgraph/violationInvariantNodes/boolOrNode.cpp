#include "atlantis/invariantgraph/violationInvariantNodes/boolOrNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/fzn/fzn_all_different_int.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/boolOr.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

BoolOrNode::BoolOrNode(InvariantGraph& graph, const VarNodeId a,
                       const VarNodeId b, const VarNodeId r)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, r) {}

BoolOrNode::BoolOrNode(InvariantGraph& graph, const VarNodeId a,
                       const VarNodeId b, const bool shouldHold)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, shouldHold) {}

void BoolOrNode::init(const InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::ranges::none_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void BoolOrNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (isReified()) {
    constraintSolver().bool_or_reif(staticInputVarNode(0).constraintVarId(),
                                    staticInputVarNode(1).constraintVarId(),
                                    reifiedVarNodeConst().constraintVarId());
  } else {
    constraintSolver().bool_or(staticInputVarNode(0).constraintVarId(),
                               staticInputVarNode(1).constraintVarId(),
                               shouldHold());
  }
}

void BoolOrNode::updateState() {
  ViolationInvariantNode::updateState();
  if (isReified()) {
    return;
  }
  if (shouldHold()) {
    const bool alwaysHolds =
        (varNodeConst(a()).isFixed() &&
         varNodeConst(a()).inDomain(bool{true})) ||
        (varNodeConst(b()).isFixed() && varNodeConst(b()).inDomain(bool{true}));
    if (alwaysHolds) {
      setState(InvariantNodeState::SUBSUMED);
    }
  } else {
    const bool alwaysHolds = varNodeConst(a()).isFixed() &&
                             varNodeConst(a()).inDomain(bool{false}) &&
                             varNodeConst(b()).isFixed() &&
                             varNodeConst(b()).inDomain(bool{false});
    if (alwaysHolds) {
      setState(InvariantNodeState::SUBSUMED);
    }
  }
}

bool BoolOrNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE && isReified() &&
         varNodeConst(a()).isFixed() != varNodeConst(b()).isFixed();
}

bool BoolOrNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }

  invariantGraph().replaceVarNode(reifiedViolationNodeId(),
                                  varNodeConst(a()).isFixed() ? b() : a());
  return true;
}

void BoolOrNode::registerOutputVars(propagation::SolverBase& solver,
                                    SolverMapping& mapping) const {
  if (violationVarId(mapping) == propagation::NULL_ID) {
    if (shouldHold()) {
      registerViolation(solver, mapping);
    } else {
      assert(!isReified());
      mapping.setIntermediateId(id(), solver.makeIntVar(0, 0, 0));
      setViolationVarId(solver.makeIntView<propagation::NotEqualConst>(
                            solver, mapping.intermediateId(id()), 0),
                        mapping);
    }
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void BoolOrNode::registerNode(propagation::SolverBase& solver,
                              SolverMapping& mapping) const {
  assert(violationVarId(mapping) != propagation::NULL_ID);
  assert(violationVarId(mapping).isVar());

  solver.makeInvariant<propagation::BoolOr>(solver, violationVarId(mapping),
                                            mapping.solverId(a()),
                                            mapping.solverId(b()));
}

std::string BoolOrNode::dotLangIdentifier() const { return "bool_or"; }

}  // namespace atlantis::invariantgraph
