#include "atlantis/invariantgraph/views/int2BoolNode.hpp"

#include <algorithm>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/int2BoolView.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

Int2BoolNode::Int2BoolNode(InvariantGraph& graph, const VarNodeId staticInput,
                           const VarNodeId output)
    : InvariantNode(graph, {output}, {staticInput}) {}

void Int2BoolNode::init(const InvariantNodeId id) {
  InvariantNode::init(id);
  assert(!invariantGraphConst()
              .varNodeConst(outputVarNodeIds().front())
              .isIntVar());
  assert(invariantGraph()
             .varNodeConst(staticInputVarNodeIds().front())
             .isIntVar());
}
void Int2BoolNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().bool2int(outputVarNodeConst(0).constraintVarId(), staticInputVarNodeConst(0).constraintVarId());
}

void Int2BoolNode::updateState() {
  invariantGraph().varNode(input()).domain()->removeBelow(Int{0});
  invariantGraph().varNode(input()).domain()->removeAbove(Int{1});

  invariantGraph()
      .varNode(outputVarNodeIds().front())
      .domain()
      ->removeBelow(Int{0});
  invariantGraph()
      .varNode(outputVarNodeIds().front())
      .domain()
      ->removeAbove(Int{1});

  if (invariantGraphConst().varNodeConst(input()).isFixed()) {
    invariantGraph()
        .varNode(outputVarNodeIds().front())
        .fixToValue(
            invariantGraphConst().varNodeConst(input()).inDomain(Int{1}));
    setState(InvariantNodeState::SUBSUMED);
  } else if (invariantGraph()
                 .varNodeConst(outputVarNodeIds().front())
                 .isFixed()) {
    invariantGraph().varNode(input()).fixToValue(
        invariantGraph()
                .varNodeConst(outputVarNodeIds().front())
                .inDomain(bool{true})
            ? Int{1}
            : Int{0});
    setState(InvariantNodeState::SUBSUMED);
  }
}

void Int2BoolNode::registerOutputVars(propagation::SolverBase& solver,
                                      SolverMapping& mapping) const {
  if (mapping.solverId(outputVarNodeIds().front()) == propagation::NULL_ID) {
    mapping.setSolverId(outputVarNodeIds().front(),
                        solver.makeIntView<propagation::Int2BoolView>(
                            solver, mapping.solverId(input())));
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void Int2BoolNode::registerNode(propagation::SolverBase&,
                                SolverMapping&) const {}

std::string Int2BoolNode::dotLangIdentifier() const { return "int2bool"; }

}  // namespace atlantis::invariantgraph
