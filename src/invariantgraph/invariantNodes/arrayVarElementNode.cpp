#include "atlantis/invariantgraph/invariantNodes/arrayVarElementNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/elementVar.hpp"

namespace atlantis::invariantgraph {

ArrayVarElementNode::ArrayVarElementNode(VarNodeId idx,
                                         std::vector<VarNodeId>&& varVector,
                                         VarNodeId output, Int offset)
    : InvariantNode({output}, {idx}, std::move(varVector)), _offset(offset) {}

void ArrayVarElementNode::init(InvariantGraph& graph,
                               const InvariantNodeId& id) {
  InvariantNode::init(graph, id);
  assert(std::all_of(staticInputVarNodeIds().begin(),
                     staticInputVarNodeIds().end(), [&](const VarNodeId& node) {
                       return graph.varNodeConst(node).isIntVar();
                     }));
  assert(std::all_of(
      dynamicInputVarNodeIds().begin(), dynamicInputVarNodeIds().end(),
      [&](const VarNodeId& node) {
        return graph.varNodeConst(outputVarNodeIds().front()).isIntVar() ==
               graph.varNodeConst(node).isIntVar();
      }));
}

bool ArrayVarElementNode::canBeReplaced(const InvariantGraph& graph) const {
  return state() == InvariantNodeState::ACTIVE &&
         graph.varNodeConst(idx()).isFixed();
}

bool ArrayVarElementNode::replace(InvariantGraph& graph) {
  if (!canBeReplaced(graph)) {
    return false;
  }
  auto& idxNode = graph.varNode(idx());
  const VarNodeId input =
      dynamicInputVarNodeIds().at(idxNode.lowerBound() - _offset);

  graph.replaceVarNode(outputVarNodeIds().front(), input);
  return true;
}

void ArrayVarElementNode::registerOutputVars(InvariantGraph& graph,
                                             propagation::SolverBase& solver) {
  makeSolverVar(solver, graph.varNode(outputVarNodeIds().front()), _offset);
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void ArrayVarElementNode::registerNode(InvariantGraph& graph,
                                       propagation::SolverBase& solver) {
  std::vector<propagation::VarId> varVector;
  std::transform(dynamicInputVarNodeIds().begin(),
                 dynamicInputVarNodeIds().end(), std::back_inserter(varVector),
                 [&](auto node) { return graph.varId(node); });

  assert(graph.varId(outputVarNodeIds().front()) != propagation::NULL_ID);

  solver.makeInvariant<propagation::ElementVar>(
      solver, graph.varId(outputVarNodeIds().front()), graph.varId(idx()),
      std::move(varVector), _offset);
}

}  // namespace atlantis::invariantgraph
