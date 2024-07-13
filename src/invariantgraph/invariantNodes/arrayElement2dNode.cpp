#include "atlantis/invariantgraph/invariantNodes/arrayElement2dNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"
#include "atlantis/propagation/invariants/element2dConst.hpp"

namespace atlantis::invariantgraph {

static std::vector<std::vector<Int>> toIntMatrix(
    std::vector<std::vector<bool>>&& boolMatrix) {
  std::vector<std::vector<Int>> intMatrix;
  intMatrix.reserve(boolMatrix.size());
  for (auto& row : boolMatrix) {
    intMatrix.emplace_back();
    intMatrix.back().reserve(row.size());
    for (bool par : row) {
      intMatrix.back().emplace_back(par ? 0 : 1);
    }
  }
  return intMatrix;
}

ArrayElement2dNode::ArrayElement2dNode(
    VarNodeId idx1, VarNodeId idx2, std::vector<std::vector<Int>>&& parMatrix,
    VarNodeId output, Int offset1, Int offset2)
    : InvariantNode({output}, {idx1, idx2}),
      _parMatrix(std::move(parMatrix)),
      _offset1(offset1),
      _offset2(offset2),
      _isIntMatrix(true) {}

ArrayElement2dNode::ArrayElement2dNode(
    VarNodeId idx1, VarNodeId idx2, std::vector<std::vector<bool>>&& parMatrix,
    VarNodeId output, Int offset1, Int offset2)
    : InvariantNode({output}, {idx1, idx2}),
      _parMatrix(toIntMatrix(std::move(parMatrix))),
      _offset1(offset1),
      _offset2(offset2),
      _isIntMatrix(false) {}

void ArrayElement2dNode::init(InvariantGraph& graph,
                              const InvariantNodeId& id) {
  InvariantNode::init(graph, id);
  assert(_isIntMatrix ==
         graph.varNodeConst(outputVarNodeIds().front()).isIntVar());
}

void ArrayElement2dNode::updateState(InvariantGraph& graph) {
  const auto& idx1Node = graph.varNodeConst(idx1());
  const auto& idx2Node = graph.varNodeConst(idx2());
  if (idx1Node.isFixed() && idx2Node.isFixed()) {
    auto& outputNode = graph.varNode(outputVarNodeIds().front());
    if (outputNode.isIntVar()) {
      outputNode.fixToValue(_parMatrix.at(idx1Node.lowerBound() - _offset1)
                                .at(idx2Node.lowerBound() - _offset2));
    } else {
      outputNode.fixToValue(_parMatrix.at(idx1Node.lowerBound() - _offset1)
                                .at(idx2Node.lowerBound() - _offset2) == 0);
    }
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayElement2dNode::canBeReplaced(
    const InvariantGraph& invariantGraph) const {
  return state() == InvariantNodeState::ACTIVE &&
         (invariantGraph.varNodeConst(idx1()).isFixed() ||
          invariantGraph.varNodeConst(idx2()).isFixed());
}

bool ArrayElement2dNode::replace(InvariantGraph& invariantGraph) {
  if (!canBeReplaced(invariantGraph)) {
    return false;
  }
  if (invariantGraph.varNode(idx1()).isFixed()) {
    assert(invariantGraph.varNode(idx1()).lowerBound() >= _offset1);
    assert(invariantGraph.varNode(idx1()).lowerBound() <
           static_cast<Int>(_parMatrix.size()) - _offset1);

    invariantGraph.addInvariantNode(std::make_unique<ArrayElementNode>(
        std::move(_parMatrix.at(invariantGraph.varNode(idx1()).lowerBound() -
                                _offset1)),
        idx2(), outputVarNodeIds().front(), _offset2));
    _parMatrix.clear();
    return true;
  }
  std::vector<Int> parMatrixRow;
  const Int col = invariantGraph.varNode(idx2()).lowerBound() - _offset2;
  assert(col >= 0);
  assert(col < static_cast<Int>(_parMatrix.front().size()));
  parMatrixRow.reserve(_parMatrix.size());
  for (const std::vector<Int>& row : _parMatrix) {
    parMatrixRow.emplace_back(row.at(col));
  }
  _parMatrix.clear();
  invariantGraph.addInvariantNode(std::make_unique<ArrayElementNode>(
      std::move(parMatrixRow), idx1(), outputVarNodeIds().front(), _offset1));
  return true;
}

void ArrayElement2dNode::registerOutputVars(InvariantGraph& invariantGraph,
                                            propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void ArrayElement2dNode::registerNode(InvariantGraph& invariantGraph,
                                      propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  solver.makeInvariant<propagation::Element2dConst>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(idx1()), invariantGraph.varId(idx2()),
      std::vector<std::vector<Int>>(_parMatrix), _offset1, _offset2);
}

}  // namespace atlantis::invariantgraph
