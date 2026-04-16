#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/elementConst.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

static Int getVal(const std::vector<Int>& parVector, const Int idx,
                  const Int offset) {
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
                                   std::vector<Int>&& parVector,
                                   const VarNodeId idx, const VarNodeId output,
                                   const Int offset)
    : InvariantNode(graph, {output}, {idx}),
      _parVector(std::move(parVector)),
      _offset(offset) {}

ArrayElementNode::ArrayElementNode(InvariantGraph& graph,
                                   std::vector<bool>&& parVector,
                                   const VarNodeId idx, const VarNodeId output,
                                   Int offset)
    : InvariantNode(graph, {output}, {idx}),
      _parVector(toIntVec(std::move(parVector))),
      _offset(offset) {}

void ArrayElementNode::init(const InvariantNodeId id) {
  InvariantNode::init(id);
  assert(staticInputVarNode(0)
             .isIntVar());
}

void ArrayElementNode::postConstraint() {
  InvariantNode::postConstraint();
  const auto& outputNode =
      outputVarNodeConst(0);
  if (outputNode.isIntVar()) {
    invariantGraph().constraintSolver().array_int_element(
        staticInputVarNode(0)
            .constraintVarId(),
        _parVector, outputNode.constraintVarId(), _offset);
  } else {
    invariantGraph().constraintSolver().array_bool_element(
        staticInputVarNode(0)
            .constraintVarId(),
        _parVector, outputNode.constraintVarId(), _offset);
  }
}

void ArrayElementNode::updateState() {
  auto& idxNode = varNode(idx());
  const auto& outputNode = outputVarNode(0);

  if (idxNode.isFixed()) {
    assert(outputNode.isFixed());
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (outputNode.isFixed()) {
    const Int val = outputNode.lowerBound();
    std::vector<Int> validIndices;
    validIndices.reserve(_parVector.size());
    for (Int i = 0; i < static_cast<Int>(_parVector.size()); ++i) {
      if (_parVector[i] == val) {
        validIndices.emplace_back(i + _offset);
      }
    }
    idxNode.domain()->removeAllValuesExcept(
        SortedUniqueVector(std::move(validIndices)));
    if (idxNode.isFixed()) {
      idxNode.setDomainType(DomainType::DOM_FIXED);
    } else if (idxNode.domain()->isInterval()) {
      idxNode.setDomainType(DomainType::DOM_RANGE);
    } else {
      idxNode.setDomainType(DomainType::DOM_DOMAIN);
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
      outputVarNodeIds(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void ArrayElementNode::registerNode(propagation::SolverBase&,
                                    SolverMapping&) const {}

std::string ArrayElementNode::dotLangIdentifier() const { return "element"; }

}  // namespace atlantis::invariantgraph
