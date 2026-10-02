#include "atlantis/invariantgraph/invariantNodes/arrayVarElementNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/elementVar.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

ArrayVarElementNode::ArrayVarElementNode(
    InvariantGraph& graph, VarNode& idx,
    std::vector<std::shared_ptr<VarNode>>&& varVector, VarNode& output,
    const Int offset)
    : InvariantNode(graph, {output}, {idx}, std::move(varVector)),
      _offset(offset) {}

void ArrayVarElementNode::init() {
  InvariantNode::init();
  assert(std::ranges::all_of(staticInputVarNodes().begin(),
                             staticInputVarNodes().end(),
                             [&](const std::shared_ptr<VarNode>& node) {
                               return varNodeConst(node).isIntVar();
                             }));
  assert(std::ranges::all_of(
      dynamicInputVarNodes().begin(), dynamicInputVarNodes().end(),
      [&](const std::shared_ptr<VarNode>& node) {
        return invariantGraph()
                   .varNodeConst(outputVarNodes().front())
                   .isIntVar() == varNodeConst(node).isIntVar();
      }));
}

void ArrayVarElementNode::postConstraint() {
  if (outputVarNodeConst(0).isIntVar()) {
    constraintSolver().array_var_int_element(
        staticInputVarNode(0).constraintVarId(),
        toConstraintVarIds(invariantGraphConst(), dynamicInputVarNodes()),
        outputVarNode(0).constraintVarId(), _offset);
  } else {
    constraintSolver().array_var_bool_element(
        staticInputVarNode(0).constraintVarId(),
        toConstraintVarIds(invariantGraphConst(), dynamicInputVarNodes()),
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
  assert(size <= static_cast<Int>(dynamicInputVarNodes().size()));
  for (Int i = static_cast<Int>(dynamicInputVarNodes().size()); i > size; --i) {
    removeDynamicInputAtIndex(dynamicInputVarNodes().size() - 1);
  }
  assert(size == static_cast<Int>(dynamicInputVarNodes().size()));
  if (staticInputVarNodeConst(0).constDomain()->isInterval()) {
    return;
  }

  std::vector<bool> indexIsSupported(dynamicInputVarNodes().size(), false);
  for (const Int val : *staticInputVarNodeConst(0).constDomain()) {
    const Int index = val - _offset;
    assert(0 <= index);
    assert(index < static_cast<Int>(dynamicInputVarNodes().size()));
    indexIsSupported[index] = true;
  }
  for (size_t i = 0; i < dynamicInputVarNodes().size(); ++i) {
    if (indexIsSupported[i]) {
      continue;
    }
    for (size_t j = i + 1; j < dynamicInputVarNodes().size(); ++j) {
      if (indexIsSupported[j] &&
          dynamicInputVarNodes()[i] == dynamicInputVarNodes()[j]) {
        indexIsSupported[i] = true;
        break;
      }
    }
  }
  assert(indexIsSupported.front());
  assert(indexIsSupported.back());
  std::shared_ptr<VarNode> prevVarNode = dynamicInputVarNodes().front();
  for (size_t i = 1; i < dynamicInputVarNodes().size(); ++i) {
    if (indexIsSupported[i]) {
      prevVarNode = dynamicInputVarNodes()[i];
      continue;
    }
    for (size_t j = i + 1; j < dynamicInputVarNodes().size(); ++j) {
      if (!indexIsSupported[j] &&
          dynamicInputVarNodes()[i] == dynamicInputVarNodes()[j]) {
        indexIsSupported[j] = true;
      }
    }
    replaceDynamicInputVarNode(dynamicInputVarNodes()[i], prevVarNode);
  }
}
bool ArrayVarElementNode::constrainsOutput(VarNode&) const {
  std::vector<Int> values;
  values.reserve(outputVarNodeConst(0).constDomain()->size());
  for (auto indexIter = staticInputVarNodeConst(0).constDomain()->begin();
       indexIter != staticInputVarNodeConst(0).constDomain()->end();
       ++indexIter) {
    const Int index = *indexIter - _offset;
    if (index < 0) {
      continue;
    }
    if (static_cast<Int>(dynamicInputVarNodes().size()) < index) {
      break;
    }
    for (long val : *dynamicInputVarNodeConst(index).constDomain()) {
      values.emplace_back(val);
    }
  }
  const SortedUniqueVector sortedVals(std::move(values));
  return !outputVarNodeConst(0).constDomain()->contains(sortedVals);
}

bool ArrayVarElementNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }

  const bool allSameVar = std::ranges::all_of(
      dynamicInputVarNodes(), [&](const std::shared_ptr<VarNode>& vId) {
        return dynamicInputVarNodes().front() == vId;
      });

  if (varNodeConst(idx()).isFixed() || allSameVar) {
    return true;
  }
  return std::ranges::all_of(dynamicInputVarNodes(),
                             [&](const std::shared_ptr<VarNode>& vId) {
                               return varNodeConst(vId).isFixed();
                             });
}

bool ArrayVarElementNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }

  const bool allSameVar = std::ranges::all_of(
      dynamicInputVarNodes(), [&](const std::shared_ptr<VarNode>& vId) {
        return dynamicInputVarNodes().front() == vId;
      });

  if (varNodeConst(idx()).isFixed() || allSameVar) {
    assert(allSameVar || dynamicInputVarNodes().size() == 1);
    if (dynamicInputVarNodes().size() > 1) {
      staticInputVarNode(0).tightenDomainType(
          staticInputVarNodeConst(0).constDomain()->isInterval()
              ? DomainType::DOM_RANGE
              : DomainType::DOM_DOMAIN);
    }
    invariantGraph().replaceVarNode(outputVarNodes().front(),
                                    dynamicInputVarNodes().front());
    return true;
  }

  assert(std::ranges::all_of(dynamicInputVarNodes(),
                             [&](const std::shared_ptr<VarNode>& vId) {
                               return varNodeConst(vId).isFixed();
                             }));

  std::vector<Int> parVector(dynamicInputVarNodes().size());
  for (size_t i = 0; i < dynamicInputVarNodes().size(); ++i) {
    parVector[i] = dynamicInputVarNodeConst(i).lowerBound();
  }
  invariantGraph().addInvariantNode(std::make_shared<ArrayElementNode>(
      invariantGraph(), std::move(parVector), idx(), outputVarNodes().front(),
      _offset));
  return true;
}

void ArrayVarElementNode::registerOutputVars(propagation::SolverBase& solver,
                                             SolverMapping& mapping) const {
  makeSolverVar(outputVarNodes().front(), _offset, solver, mapping);
  assert(std::ranges::all_of(outputVarNodes().begin(), outputVarNodes().end(),
                             [&](const std::shared_ptr<VarNode>& vId) {
                               return mapping.solverId(vId) !=
                                      propagation::NULL_ID;
                             }));
}

void ArrayVarElementNode::registerNode(propagation::SolverBase& solver,
                                       SolverMapping& mapping) const {
  std::vector<propagation::VarViewId> varVector;
  std::ranges::transform(dynamicInputVarNodes(), std::back_inserter(varVector),
                         [&](auto nId) { return mapping.solverId(nId); });

  assert(mapping.solverId(outputVarNodes().front()) != propagation::NULL_ID);
  assert(mapping.solverId(outputVarNodes().front()).isVar());

  solver.makeInvariant<propagation::ElementVar>(
      solver, mapping.solverId(outputVarNodes().front()),
      mapping.solverId(idx()), std::move(varVector), _offset);
}

std::string ArrayVarElementNode::dotLangIdentifier() const {
  return "var_element";
}

}  // namespace atlantis::invariantgraph
