#include "atlantis/invariantgraph/invariantNodes/arrayVarElementNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/elementVar.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

ArrayVarElementNode::ArrayVarElementNode(InvariantGraph& graph,
                                         const VarNodeId idx,
                                         std::vector<VarNodeId>&& varVector,
                                         const VarNodeId output,
                                         const Int offset)
    : InvariantNode(graph, {output}, {idx}, std::move(varVector)),
      _offset(offset) {}

void ArrayVarElementNode::init(const InvariantNodeId id) {
  InvariantNode::init(id);
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId node) { return varNodeConst(node).isIntVar(); }));
  assert(std::ranges::all_of(
      dynamicInputVarNodeIds().begin(), dynamicInputVarNodeIds().end(),
      [&](const VarNodeId node) {
        return invariantGraph()
                   .varNodeConst(outputVarNodeIds().front())
                   .isIntVar() == varNodeConst(node).isIntVar();
      }));
}

void ArrayVarElementNode::postConstraint() {
  std::vector<ConstraintVarId> inputs(dynamicInputVarNodeIds().size(),
                                      ConstraintVarId{NULL_NODE_ID});
  for (size_t i = 0; i < dynamicInputVarNodeIds().size(); ++i) {
    inputs[i] = dynamicInputVarNode(i).constraintVarId();
  }
  if (outputVarNodeConst(0).isIntVar()) {
    constraintSolver().array_var_int_element(
        staticInputVarNode(0).constraintVarId(), inputs,
        outputVarNode(0).constraintVarId(), _offset);
  } else {
    constraintSolver().array_var_bool_element(
        staticInputVarNode(0).constraintVarId(), inputs,
        outputVarNode(0).constraintVarId(), _offset);
  }
}

void ArrayVarElementNode::updateState() {
  InvariantNode::updateState();

  // remove invalid vars from front:
  const Int lb = staticInputVarNodeConst(0).lowerBound();
  assert(_offset <= lb);
  for (Int i = _offset; i < lb; ++i) {
    removeDynamicInputAtIndex(0);
  }
  _offset = lb;

  // Remove invalid vars from end:
  const Int size = staticInputVarNodeConst(0).upperBound() -
                   staticInputVarNodeConst(0).lowerBound() + 1;
  assert(size <= static_cast<Int>(dynamicInputVarNodeIds().size()));
  for (Int i = static_cast<Int>(dynamicInputVarNodeIds().size()); i > size;
       --i) {
    removeDynamicInputAtIndex(dynamicInputVarNodeIds().size() - 1);
  }
  assert(size == static_cast<Int>(dynamicInputVarNodeIds().size()));
  if (staticInputVarNodeConst(0).constDomain()->isInterval()) {
    return;
  }

  std::vector<bool> indexIsSupported(dynamicInputVarNodeIds().size(), false);
  for (auto iter = staticInputVarNodeConst(0).constDomain()->begin();
       iter != staticInputVarNodeConst(0).constDomain()->end(); ++iter) {
    const Int index = *iter - _offset;
    assert(0 <= index);
    assert(index < static_cast<Int>(dynamicInputVarNodeIds().size()));
    indexIsSupported[index] = true;
  }
  for (size_t i = 0; i < dynamicInputVarNodeIds().size(); ++i) {
    if (indexIsSupported[i]) {
      continue;
    }
    for (size_t j = i + 1; j < dynamicInputVarNodeIds().size(); ++j) {
      if (indexIsSupported[j] &&
          dynamicInputVarNodeIds()[i] == dynamicInputVarNodeIds()[j]) {
        indexIsSupported[i] = true;
        break;
      }
    }
  }
  assert(indexIsSupported.front());
  assert(indexIsSupported.back());
  VarNodeId prevVarNodeId = dynamicInputVarNodeIds().front();
  for (size_t i = 1; i < dynamicInputVarNodeIds().size(); ++i) {
    if (indexIsSupported[i]) {
      prevVarNodeId = dynamicInputVarNodeIds()[i];
      continue;
    }
    for (size_t j = i + 1; j < dynamicInputVarNodeIds().size(); ++j) {
      if (!indexIsSupported[j] &&
          dynamicInputVarNodeIds()[i] == dynamicInputVarNodeIds()[j]) {
        indexIsSupported[j] = true;
      }
    }
    replaceDynamicInputVarNode(staticInputVarNodeIds()[i], prevVarNodeId);
  }
}

bool ArrayVarElementNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }

  const bool allSameVar =
      std::ranges::all_of(dynamicInputVarNodeIds(), [&](const VarNodeId vId) {
        return dynamicInputVarNodeIds().front() == vId;
      });

  if (varNodeConst(idx()).isFixed() || allSameVar) {
    return true;
  }
  return std::ranges::all_of(
      dynamicInputVarNodeIds(),
      [&](const VarNodeId vId) { return varNodeConst(vId).isFixed(); });
}

bool ArrayVarElementNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }

  const bool allSameVar =
      std::ranges::all_of(dynamicInputVarNodeIds(), [&](const VarNodeId vId) {
        return dynamicInputVarNodeIds().front() == vId;
      });

  if (varNodeConst(idx()).isFixed() || allSameVar) {
    assert(allSameVar || dynamicInputVarNodeIds().size() == 1);
    if (dynamicInputVarNodeIds().size() > 1) {
      staticInputVarNode(0).tightenDomainType(
          staticInputVarNodeConst(0).constDomain()->isInterval()
              ? DomainType::DOM_RANGE
              : DomainType::DOM_DOMAIN);
    }
    invariantGraph().replaceVarNode(outputVarNodeIds().front(),
                                    dynamicInputVarNodeIds().front());
    return true;
  }

  assert(std::ranges::all_of(
      dynamicInputVarNodeIds(),
      [&](const VarNodeId vId) { return varNodeConst(vId).isFixed(); }));

  std::vector<Int> parVector(dynamicInputVarNodeIds().size());
  for (size_t i = 0; i < dynamicInputVarNodeIds().size(); ++i) {
    parVector[i] = dynamicInputVarNodeConst(i).lowerBound();
  }
  invariantGraph().addInvariantNode(std::make_shared<ArrayElementNode>(
      invariantGraph(), std::move(parVector), idx(), outputVarNodeIds().front(),
      _offset));
  return true;
}

void ArrayVarElementNode::registerOutputVars(propagation::SolverBase& solver,
                                             SolverMapping& mapping) const {
  makeSolverVar(outputVarNodeIds().front(), _offset, solver, mapping);
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void ArrayVarElementNode::registerNode(propagation::SolverBase& solver,
                                       SolverMapping& mapping) const {
  std::vector<propagation::VarViewId> varVector;
  std::ranges::transform(dynamicInputVarNodeIds(),
                         std::back_inserter(varVector),
                         [&](auto nId) { return mapping.solverId(nId); });

  assert(mapping.solverId(outputVarNodeIds().front()) != propagation::NULL_ID);
  assert(mapping.solverId(outputVarNodeIds().front()).isVar());

  solver.makeInvariant<propagation::ElementVar>(
      solver, mapping.solverId(outputVarNodeIds().front()),
      mapping.solverId(idx()), std::move(varVector), _offset);
}

std::string ArrayVarElementNode::dotLangIdentifier() const {
  return "var_element";
}

}  // namespace atlantis::invariantgraph
