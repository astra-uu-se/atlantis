#include "atlantis/invariantgraph/violationInvariantNodes/inIntervalNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/fzn/fzn_all_different_int.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/inIntervalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

InIntervalNode::InIntervalNode(InvariantGraph& graph, VarNodeId input, Int lb,
                               Int ub, VarNodeId r)
    : ViolationInvariantNode(graph, {input}, r), _lb(lb), _ub(ub) {}

InIntervalNode::InIntervalNode(InvariantGraph& graph, VarNodeId input, Int lb,
                               Int ub, bool shouldHold)
    : ViolationInvariantNode(graph, {input}, shouldHold), _lb(lb), _ub(ub) {}

void InIntervalNode::init(InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void InIntervalNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (isReified()) {
    return invariantGraph().constraintSolver().set_in_reif(
        staticInputVarNodeConst(0).constraintVarId(), _lb, _ub,
        reifiedVarNodeConst().constraintVarId());
  }
  invariantGraph().constraintSolver().set_in(
      staticInputVarNodeConst(0).constraintVarId(), _lb, _ub, shouldHold());
}

void InIntervalNode::updateState() {
  ViolationInvariantNode::updateState();
  if (!isReified()) {
    staticInputVarNode(0).tightenDomainType();
    setState(InvariantNodeState::SUBSUMED);
  }
}

void InIntervalNode::registerOutputVars(propagation::SolverBase& solver,
                                        SolverMapping& mapping) const {
  if (violationVarId(mapping) == propagation::NULL_ID) {
    if (shouldHold()) {
      setViolationVarId(
          solver.makeIntView<propagation::InIntervalConst>(
              solver, mapping.solverId(staticInputVarNodeIds().front()), _lb,
              _ub),
          mapping);
    } else {
      assert(!isReified());
      mapping.setIntermediateId(
          id(), solver.makeIntView<propagation::InIntervalConst>(
                    solver, mapping.solverId(staticInputVarNodeIds().front()),
                    _lb, _ub));
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

void InIntervalNode::registerNode(propagation::SolverBase&,
                                  SolverMapping&) const {}

std::string InIntervalNode::dotLangIdentifier() const { return "in_interval"; }

}  // namespace atlantis::invariantgraph
