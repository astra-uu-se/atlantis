#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
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
                                   std::vector<Int>&& parVector, VarNode& idx,
                                   VarNode& output, const Int offset)
    : InvariantNode(graph, {output.ptr()}, {idx.ptr()}),
      _parVector(std::move(parVector)),
      _offset(offset) {}

ArrayElementNode::ArrayElementNode(InvariantGraph& graph,
                                   std::vector<bool>&& parVector, VarNode& idx,
                                   VarNode& output, const Int offset)
    : InvariantNode(graph, {output.ptr()}, {idx.ptr()}),
      _parVector(toIntVec(std::move(parVector))),
      _offset(offset) {}

void ArrayElementNode::init() {
  InvariantNode::init();
  assert(_staticInputVarNodes.front()->isIntVar());
}

void ArrayElementNode::postConstraint() {
  InvariantNode::postConstraint();
  const auto& outputNode = _outputVarNodes.front();
  if (outputNode->isIntVar()) {
    invariantGraph().constraintSolver().array_int_element(
        _staticInputVarNodes.front()->constraintVarId(), _parVector,
        outputNode->constraintVarId(), _offset);
  } else {
    std::vector<bool> boolVector(_parVector.size());
    for (size_t i = 0; i < boolVector.size(); ++i) {
      boolVector[i] = _parVector[i] == 0;
    }
    invariantGraph().constraintSolver().array_bool_element(
        _staticInputVarNodes.front()->constraintVarId(), boolVector,
        outputNode->constraintVarId(), _offset);
  }
}

void ArrayElementNode::updateState() {
  if (_staticInputVarNodes.front()->isFixed()) {
    assert(_outputVarNodes.front()->isFixed());
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (_outputVarNodes.front()->isFixed()) {
    const Int val = _outputVarNodes.front()->lowerBound();
    std::vector<Int> validIndices;
    validIndices.reserve(_parVector.size());
    for (Int i = 0; i < static_cast<Int>(_parVector.size()); ++i) {
      if (_parVector[i] == val) {
        validIndices.emplace_back(i + _offset);
      }
    }
    _staticInputVarNodes.front()->domain()->removeAllValuesExcept(
        SortedUniqueVector(std::move(validIndices)));
    _staticInputVarNodes.front()->tightenDomainType();
    setState(InvariantNodeState::SUBSUMED);
  }
}
bool ArrayElementNode::constrainsOutput(const VarNode&) const {
  std::vector<Int> values;
  values.reserve(_parVector.size());
  for (auto iter = _staticInputVarNodes.front()->constDomain()->begin();
       iter != _staticInputVarNodes.front()->constDomain()->end(); ++iter) {
    const Int index = *iter - _offset;
    if (index < 0) {
      continue;
      ;
    }
    if (static_cast<Int>(_parVector.size()) < index) {
      break;
    }
    values.emplace_back(_parVector[index]);
  }
  const SortedUniqueVector sortedVals(std::move(values));
  return !_outputVarNodes.front()->constDomain()->contains(sortedVals);
}

void ArrayElementNode::registerOutputVars(propagation::SolverBase& solver,
                                          SolverMapping& mapping) const {
  if (mapping.solverId(_outputVarNodes.front()) == propagation::NULL_ID) {
    assert(mapping.solverId(staticInputVarNodes().front()) !=
           propagation::NULL_ID);
    mapping.setSolverId(
        _outputVarNodes.front(),
        solver.makeIntView<propagation::ElementConst>(
            solver, mapping.solverId(staticInputVarNodes().front()),
            std::vector<Int>(_parVector), _offset));
  }
  assert(std::ranges::all_of(
      _outputVarNodes, [&](const std::shared_ptr<VarNode>& vNode) {
        return mapping.solverId(vNode) != propagation::NULL_ID;
      }));
}

void ArrayElementNode::registerNode(propagation::SolverBase&,
                                    SolverMapping&) const {}

std::string ArrayElementNode::dotLangIdentifier() const { return "element"; }

}  // namespace atlantis::invariantgraph
