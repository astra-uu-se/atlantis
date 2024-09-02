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
    IInvariantGraph& graph, VarNodeId idx1, VarNodeId idx2,
    std::vector<std::vector<Int>>&& parMatrix, VarNodeId output, Int offset1,
    Int offset2)
    : InvariantNode(graph, {output}, {idx1, idx2}),
      _parMatrix(std::move(parMatrix)),
      _offset1(offset1),
      _offset2(offset2),
      _isIntMatrix(true) {}

ArrayElement2dNode::ArrayElement2dNode(
    IInvariantGraph& graph, VarNodeId idx1, VarNodeId idx2,
    std::vector<std::vector<bool>>&& parMatrix, VarNodeId output, Int offset1,
    Int offset2)
    : InvariantNode(graph, {output}, {idx1, idx2}),
      _parMatrix(toIntMatrix(std::move(parMatrix))),
      _offset1(offset1),
      _offset2(offset2),
      _isIntMatrix(false) {}

void ArrayElement2dNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(_isIntMatrix == invariantGraphConst()
                             .varNodeConst(outputVarNodeIds().front())
                             .isIntVar());
}

void ArrayElement2dNode::updateState() {
  const auto& idx1Node = invariantGraphConst().varNodeConst(idx1());
  const auto& idx2Node = invariantGraphConst().varNodeConst(idx2());
  if (idx1Node.isFixed() && idx2Node.isFixed()) {
    auto& outputNode = invariantGraph().varNode(outputVarNodeIds().front());
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

bool ArrayElement2dNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         (invariantGraphConst().varNodeConst(idx1()).isFixed() ||
          invariantGraphConst().varNodeConst(idx2()).isFixed());
}

bool ArrayElement2dNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (invariantGraph().varNode(idx1()).isFixed()) {
    const Int rowIndex =
        invariantGraph().varNode(idx1()).lowerBound() - _offset1;
    assert(rowIndex >= 0);
    assert(rowIndex < static_cast<Int>(_parMatrix.size()));

    invariantGraph().addInvariantNode(std::make_shared<ArrayElementNode>(
        invariantGraph(), std::move(_parMatrix.at(rowIndex)), idx2(),
        outputVarNodeIds().front(), _offset2, _isIntMatrix));
    _parMatrix.clear();
    return true;
  }
  std::vector<Int> parMatrixRow;
  const Int colIndex = invariantGraph().varNode(idx2()).lowerBound() - _offset2;
  assert(colIndex >= 0);
  assert(colIndex < static_cast<Int>(_parMatrix.front().size()));
  parMatrixRow.reserve(_parMatrix.size());
  for (const std::vector<Int>& row : _parMatrix) {
    parMatrixRow.emplace_back(row.at(colIndex));
  }
  _parMatrix.clear();
  invariantGraph().addInvariantNode(std::make_shared<ArrayElementNode>(
      invariantGraph(), std::move(parMatrixRow), idx1(),
      outputVarNodeIds().front(), _offset1, _isIntMatrix));
  return true;
}

void ArrayElement2dNode::registerOutputVars() {
  if (!staticInputVarNodeIds().empty()) {
    makeSolverVar(outputVarNodeIds().front());
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void ArrayElement2dNode::registerNode() {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  assert(invariantGraph().varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  assert(invariantGraph().varId(outputVarNodeIds().front()).isVar());

  solver().makeInvariant<propagation::Element2dConst>(
      solver(), invariantGraph().varId(outputVarNodeIds().front()),
      invariantGraph().varId(idx1()), invariantGraph().varId(idx2()),
      std::vector<std::vector<Int>>(_parMatrix), _offset1, _offset2);
}

std::string ArrayElement2dNode::dotLangIdentifier() const {
  return "element2d";
}

}  // namespace atlantis::invariantgraph
