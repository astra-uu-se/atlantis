#include "atlantis/invariantgraph/violationInvariantNodes/setInNode.hpp"

#include <boost/xpressive/detail/core/access.hpp>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/fzn/fzn_all_different_int.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/inDomain.hpp"
#include "atlantis/propagation/views/inIntervalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

SetInNode::SetInNode(InvariantGraph& graph, const VarNodeId input,
                     std::vector<Int>&& values, const VarNodeId r)
    : ViolationInvariantNode(graph, {input}, r), _values(std::move(values)) {}

SetInNode::SetInNode(InvariantGraph& graph, const VarNodeId input,
                     std::vector<Int>&& values, const bool shouldHold)
    : ViolationInvariantNode(graph, {input}, shouldHold),
      _values(std::move(values)) {}

void SetInNode::init(const InvariantNodeId id) {
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
void SetInNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (isReified()) {
    return constraintSolver().set_in_reif(
        staticInputVarNodeConst(0).constraintVarId(), _values,
        reifiedVarNodeConst().constraintVarId());
  }
  constraintSolver().set_in(staticInputVarNodeConst(0).constraintVarId(),
                            _values, shouldHold());
}

void SetInNode::updateState() {
  ViolationInvariantNode::updateState();
  if (!isReified()) {
    setState(InvariantNodeState::SUBSUMED);
    staticInputVarNode(0).tightenDomainType();
  }
}

void SetInNode::registerOutputVars(propagation::SolverBase& solver,
                                   SolverMapping& mapping) const {
  assert(isReified());
  if (violationVarId(mapping) == propagation::NULL_ID) {
    const propagation::VarViewId input =
        mapping.solverId(staticInputVarNodeIds().front());
    if (_values->size() == 1) {
      setViolationVarId(
          makeSolverConstIntRelation(solver, input, RelationType::REL_TYPE_EQ,
                                     _values->front(), shouldHold()),
          mapping);
    } else if (_values.isInterval()) {
      if (!shouldHold()) {
        mapping.setIntermediateId(
            id(), solver.makeIntView<propagation::InIntervalConst>(
                      solver, input, _values->front(), _values->back()));
        setViolationVarId(
            makeSolverConstBoolRelation(solver, mapping.intermediateId(id()),
                                        RelationType::REL_TYPE_EQ, false),
            mapping);
      } else {
        setViolationVarId(solver.makeIntView<propagation::InIntervalConst>(
                              solver, input, _values->front(), _values->back()),
                          mapping);
      }
    } else {
      std::vector<DomainEntry> domainEntries;
      domainEntries.reserve(_values->size());
      std::ranges::transform(
          *_values, std::back_inserter(domainEntries),
          [](const auto& value) { return DomainEntry(value, value); });

      if (!shouldHold()) {
        mapping.setIntermediateId(id(),
                                  solver.makeIntView<propagation::InDomain>(
                                      solver, input, std::move(domainEntries)));
        setViolationVarId(
            makeSolverConstBoolRelation(solver, mapping.intermediateId(id()),
                                        RelationType::REL_TYPE_EQ, false),
            mapping);
      } else {
        setViolationVarId(solver.makeIntView<propagation::InDomain>(
                              solver, input, std::move(domainEntries)),
                          mapping);
      }
    }
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void SetInNode::registerNode(propagation::SolverBase&, SolverMapping&) const {}

std::string SetInNode::dotLangIdentifier() const { return "set_in"; }

}  // namespace atlantis::invariantgraph
