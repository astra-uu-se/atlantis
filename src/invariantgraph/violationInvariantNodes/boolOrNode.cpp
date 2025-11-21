#include "atlantis/invariantgraph/violationInvariantNodes/boolOrNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/fzn/fzn_all_different_int.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/boolOr.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

BoolOrNode::BoolOrNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                       VarNodeId r)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, r) {}

BoolOrNode::BoolOrNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                       bool shouldHold)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, shouldHold) {}

void BoolOrNode::init(InvariantNodeId id) {
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
