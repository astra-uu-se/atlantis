#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/elementConst.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

static Int getVal(const std::vector<Int>& parVector, Int idx, Int offset) {
  assert(0 <= idx - offset &&
         idx - offset < static_cast<Int>(parVector.size()));
  return parVector.at(idx - offset);
}

static std::vector<Int> toIntVec(std::vector<bool>&& boolVec) {
  std::vector<Int> intVec(boolVec.size());
  for (size_t i = 0; i < boolVec.size(); ++i) {
    intVec[i] = boolVec[i] ? 0 : 1;
  }
  return intVec;
}

ArrayElementNode::ArrayElementNode(InvariantGraph& graph,
                                   std::vector<Int>&& parVector, VarNodeId idx,
                                   VarNodeId output, Int offset)
    : InvariantNode(graph, {output}, {idx}),
      _parVector(std::move(parVector)),
      _offset(offset) {}

ArrayElementNode::ArrayElementNode(InvariantGraph& graph,
                                   std::vector<bool>&& parVector, VarNodeId idx,
                                   VarNodeId output, Int offset)
    : InvariantNode(graph, {output}, {idx}),
      _parVector(toIntVec(std::move(parVector))),
      _offset(offset) {}

void ArrayElementNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(invariantGraphConst()
             .varNodeConst(staticInputVarNodeIds().front())
             .isIntVar());
}

void ArrayElementNode::updateState() {
  auto& idxNode = invariantGraph().varNode(idx());
  auto& outputNode = invariantGraph().varNode(outputVarNodeIds().front());

  idxNode.removeValuesBelow(_offset);
  idxNode.removeValuesAbove(_offset + static_cast<Int>(_parVector.size()) - 1);

  if (idxNode.isFixed()) {
    if (outputNode.isIntVar()) {
      outputNode.fixToValue(getVal(_parVector, idxNode.lowerBound(), _offset));
    } else {
      outputNode.fixToValue(getVal(_parVector, idxNode.lowerBound(), _offset) ==
                            0);
    }
    setState(InvariantNodeState::SUBSUMED);
  }
  if (outputNode.isFixed()) {
    const Int val = outputNode.lowerBound();
    std::vector<Int> valsToRemove;
    valsToRemove.reserve(idxNode.domain()->size());
    for (const Int index : *idxNode.constDomain()) {
      if (getVal(_parVector, index, _offset) != val) {
        valsToRemove.emplace_back(index);
      }
    }
    idxNode.domain()->remove(SortedUniqueVector(std::move(valsToRemove)));
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  const Int val = getVal(_parVector, idxNode.lowerBound(), _offset);
  const bool allSameVal = std::all_of(
      idxNode.domain()->begin(), idxNode.domain()->end(), [&](const Int index) {
        return getVal(_parVector, index, _offset) == val;
      });
  if (allSameVal) {
    if (outputNode.isIntVar()) {
      outputNode.fixToValue(val);
    } else {
      outputNode.fixToValue(bool{val == 0});
    }
    setState(InvariantNodeState::SUBSUMED);
  }
}

void ArrayElementNode::registerOutputVars(propagation::SolverBase& solver,
                                          SolverMapping& mapping) const {
  if (mapping.solverId(outputVarNodeIds().front()) == propagation::NULL_ID) {
    assert(mapping.solverId(idx()) != propagation::NULL_ID);
    mapping.setSolverId(outputVarNodeIds().front(),
                        solver.makeIntView<propagation::ElementConst>(
                            solver, mapping.solverId(idx()),
                            std::vector<Int>(_parVector), _offset));
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void ArrayElementNode::registerNode(propagation::SolverBase&,
                                    SolverMapping&) const {}

std::string ArrayElementNode::dotLangIdentifier() const { return "element"; }

}  // namespace atlantis::invariantgraph
