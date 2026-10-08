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

InIntervalNode::InIntervalNode(InvariantGraph& graph, VarNode& input,
                               const Int lb, const Int ub, const VarNode& r)
    : ViolationInvariantNode(graph, {input}, r), _lb(lb), _ub(ub) {}

InIntervalNode::InIntervalNode(InvariantGraph& graph, VarNode& input,
                               const Int lb, const Int ub,
                               const bool shouldHold)
    : ViolationInvariantNode(graph, {input}, shouldHold), _lb(lb), _ub(ub) {}

void InIntervalNode::init() {
  ViolationInvariantNode::init();
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNode()).isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodes().begin(), staticInputVarNodes().end(),
      [&](const std::shared_ptr<VarNode>& vId) { return vId.isIntVar(); }));
}

void InIntervalNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (isReified()) {
    return invariantGraph().constraintSolver().set_in_reif(
        staticInputVarNodes().front().constraintVarId(), _lb, _ub,
        reifiedVarNodeConst().constraintVarId());
  }
  invariantGraph().constraintSolver().set_in(
      staticInputVarNodes().front().constraintVarId(), _lb, _ub, shouldHold());
}

void InIntervalNode::updateState() {
  ViolationInvariantNode::updateState();
  if (!isReified()) {
    staticInputVarNodes().front().tightenDomainType();
    setState(InvariantNodeState::SUBSUMED);
  }
}

void InIntervalNode::registerOutputVars(propagation::SolverBase& solver,
                                        SolverMapping& mapping) const {
  if (violationVarId(mapping) == propagation::NULL_ID) {
    if (shouldHold()) {
      setViolationVarId(
          solver.makeIntView<propagation::InIntervalConst>(
              solver, mapping.solverId(staticInputVarNodes().front()), _lb,
              _ub),
          mapping);
    } else {
      assert(!isReified());
      mapping.setIntermediateId(
          TODO, solver.makeIntView<propagation::InIntervalConst>(
                    solver, mapping.solverId(staticInputVarNodes().front()),
                    _lb, _ub));
      setViolationVarId(solver.makeIntView<propagation::NotEqualConst>(
                            solver, mapping.intermediateId(TODO), 0),
                        mapping);
    }
  }
  assert(std::ranges::all_of(outputVarNodes().begin(), outputVarNodes().end(),
                             [&](const std::shared_ptr<VarNode>& vId) {
                               return mapping.solverId(vId) !=
                                      propagation::NULL_ID;
                             }));
}

void InIntervalNode::registerNode(propagation::SolverBase&,
                                  SolverMapping&) const {}

std::string InIntervalNode::dotLangIdentifier() const { return "in_interval"; }

}  // namespace atlantis::invariantgraph
