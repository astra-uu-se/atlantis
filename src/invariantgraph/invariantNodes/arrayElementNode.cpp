#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/elementConst.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

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
                                   const Int offset)
    : InvariantNode(graph, {output}, {idx}),
      _parVector(toIntVec(std::move(parVector))),
      _offset(offset) {}

void ArrayElementNode::init(const InvariantNodeId id) {
  InvariantNode::init(id);
  assert(staticInputVarNode(0).isIntVar());
}

void ArrayElementNode::postConstraint() {
  InvariantNode::postConstraint();
  const auto& outputNode = outputVarNodeConst(0);
  if (outputNode.isIntVar()) {
    invariantGraph().constraintSolver().array_int_element(
        staticInputVarNode(0).constraintVarId(), _parVector,
        outputNode.constraintVarId(), _offset);
  } else {
    std::vector<bool> boolVector(_parVector.size());
    for (size_t i = 0; i < boolVector.size(); ++i) {
      boolVector[i] = _parVector[i] == 0;
    }
    invariantGraph().constraintSolver().array_bool_element(
        staticInputVarNode(0).constraintVarId(), boolVector,
        outputNode.constraintVarId(), _offset);
  }
}

void ArrayElementNode::updateState() {
  if (staticInputVarNodeConst(0).isFixed()) {
    assert(outputVarNodeConst(0).isFixed());
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (outputVarNodeConst(0).isFixed()) {
    const Int val = outputVarNodeConst(0).lowerBound();
    std::vector<Int> validIndices;
    validIndices.reserve(_parVector.size());
    for (Int i = 0; i < static_cast<Int>(_parVector.size()); ++i) {
      if (_parVector[i] == val) {
        validIndices.emplace_back(i + _offset);
      }
    }
    staticInputVarNode(0).domain()->removeAllValuesExcept(
        SortedUniqueVector(std::move(validIndices)));
    if (staticInputVarNodeConst(0).isFixed()) {
      staticInputVarNode(0).setDomainType(DomainType::DOM_FIXED);
    } else if (staticInputVarNode(0).domain()->isInterval()) {
      staticInputVarNode(0).setDomainType(DomainType::DOM_RANGE);
    } else {
      staticInputVarNode(0).setDomainType(DomainType::DOM_DOMAIN);
    }
    setState(InvariantNodeState::SUBSUMED);
  }
}

void ArrayElementNode::registerOutputVars(propagation::SolverBase& solver,
                                          SolverMapping& mapping) const {
  if (mapping.solverId(outputVarNodeIds().front()) == propagation::NULL_ID) {
    assert(mapping.solverId(staticInputVarNodeIds().front()) !=
           propagation::NULL_ID);
    mapping.setSolverId(
        outputVarNodeIds().front(),
        solver.makeIntView<propagation::ElementConst>(
            solver, mapping.solverId(staticInputVarNodeIds().front()),
            std::vector<Int>(_parVector), _offset));
  }
  assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
    return mapping.solverId(vId) != propagation::NULL_ID;
  }));
}

void ArrayElementNode::registerNode(propagation::SolverBase&,
                                    SolverMapping&) const {}

std::string ArrayElementNode::dotLangIdentifier() const { return "element"; }

}  // namespace atlantis::invariantgraph
