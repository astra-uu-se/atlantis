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
  /*
  idxNode.removeValuesBelow(_offset);
  idxNode.removeValuesAbove(
      _offset + static_cast<Int>(dynamicInputVarNodeIds().size()) - 1);
  */

  const Int overflow =
      _offset +
      static_cast<Int>(static_cast<Int>(dynamicInputVarNodeIds().size())) -
      idxNode.upperBound() - 1;

  const Int underflow = idxNode.lowerBound() - _offset;

  if (overflow <= 0 && underflow <= 0) {
    return;
  }

  std::vector<VarNodeId> varNodeIdsToRemove;
  varNodeIdsToRemove.reserve(overflow + underflow);

  for (Int i = static_cast<Int>(dynamicInputVarNodeIds().size()) - overflow;
       i < static_cast<Int>(dynamicInputVarNodeIds().size()); ++i) {
    varNodeIdsToRemove.emplace_back(dynamicInputVarNodeIds().at(i));
  }

  for (Int i = 0; i < underflow; ++i) {
    varNodeIdsToRemove.emplace_back(dynamicInputVarNodeIds().at(i));
  }

  _offset += underflow;

  _dynamicInputVarNodeIds.erase(_dynamicInputVarNodeIds.end() - overflow,
                                _dynamicInputVarNodeIds.end());
  _dynamicInputVarNodeIds.erase(_dynamicInputVarNodeIds.begin(),
                                _dynamicInputVarNodeIds.begin() + underflow);

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

std::string ArrayVarElementNode::dotLangIdentifier() const {
  return "var_element";
}

}  // namespace atlantis::invariantgraph
