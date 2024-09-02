#include "atlantis/invariantgraph/violationInvariantNodes/inIntervalNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/views/inIntervalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/utils/variant.hpp"

namespace atlantis::invariantgraph {

InIntervalNode::InIntervalNode(VarNodeId input, Int lb, Int ub, VarNodeId r)
    : ViolationInvariantNode({input}, r), _lb(lb), _ub(ub) {}

InIntervalNode::InIntervalNode(VarNodeId input, Int lb, Int ub, bool shouldHold)
    : ViolationInvariantNode({input}, shouldHold), _lb(lb), _ub(ub) {}

void InIntervalNode::init(const InvariantNodeId& id) {
  ViolationInvariantNode::init(id);
  assert(!isReified() ||
         !graph.varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::all_of(staticInputVarNodeIds().begin(),
                     staticInputVarNodeIds().end(), [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).isIntVar();
                     }));
}

void InIntervalNode::registerOutputVars() {
  if (violationVarId(graph) == propagation::NULL_ID) {
    if (shouldHold()) {
      setViolationVarId(
          graph,
          solver.makeIntView<propagation::InIntervalConst>(
              solver, graph.varId(staticInputVarNodeIds().front()), _lb, _ub));
    } else {
      assert(!isReified());
      _intermediate = solver.makeIntView<propagation::InIntervalConst>(
          solver, graph.varId(staticInputVarNodeIds().front()), _lb, _ub);
      setViolationVarId(graph, solver.makeIntView<propagation::NotEqualConst>(
                                   solver, _intermediate, 0));
    }
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void InIntervalNode::registerNode() {}

}  // namespace atlantis::invariantgraph
