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

void ArrayVarElementNode::updateState(InvariantGraph& graph) {
  VarNode& idxNode = graph.varNode(idx());
  // idxNode.removeValuesBelow(_offset);
  // idxNode.removeValuesAbove(
  //     _offset + static_cast<Int>(dynamicInputVarNodeIds().size()) - 1);

  std::vector<VarNodeId> varNodeIdsToRemove;
  varNodeIdsToRemove.reserve(dynamicInputVarNodeIds().size());

  VarNodeId placeholder{NULL_NODE_ID};

  for (Int val = _offset;
       val < static_cast<Int>(_dynamicInputVarNodeIds.size()) + _offset;
       ++val) {
    assert(val - _offset < static_cast<Int>(dynamicInputVarNodeIds().size()));
    if (idxNode.inDomain(val) ||
        graph.varNodeConst(dynamicInputVarNodeIds().at(val - _offset))
            .isFixed()) {
      continue;
    }

    varNodeIdsToRemove.emplace_back(_dynamicInputVarNodeIds.at(val - _offset));

    if (placeholder == NULL_NODE_ID) {
      placeholder = graph.varNodeConst(outputVarNodeIds().front()).isIntVar()
                        ? graph.retrieveIntVarNode(0)
                        : graph.retrieveBoolVarNode(false);
    }
    _dynamicInputVarNodeIds.at(val - _offset) = placeholder;
  }

  if (placeholder != NULL_NODE_ID &&
      std::none_of(
          graph.varNodeConst(placeholder).dynamicInputTo().begin(),
          graph.varNodeConst(placeholder).dynamicInputTo().end(),
          [&](const InvariantNodeId& invId) { return invId == id(); })) {
    graph.varNode(placeholder).markAsInputFor(id(), false);
  }

  for (const auto& vId : varNodeIdsToRemove) {
    if (std::none_of(dynamicInputVarNodeIds().begin(),
                     dynamicInputVarNodeIds().end(),
                     [&](const VarNodeId& dId) { return dId == vId; })) {
      removeDynamicInputVarNode(graph.varNode(vId));
    }
  }
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
