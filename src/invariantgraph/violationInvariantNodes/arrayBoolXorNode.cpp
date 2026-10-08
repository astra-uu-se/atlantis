#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolXorNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolAllEqualNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolRelNode.hpp"
#include "atlantis/propagation/invariants/boolLinear.hpp"
#include "atlantis/propagation/invariants/boolXor.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

ArrayBoolXorNode::ArrayBoolXorNode(InvariantGraph& graph, VarNode& a,
                                   VarNode& b, VarNode& reified)
    : ArrayBoolXorNode(graph, std::vector<std::shared_ptr<VarNode>>{a.ptr(), b.ptr()},
                       reified) {}

ArrayBoolXorNode::ArrayBoolXorNode(InvariantGraph& graph, VarNode& a,
                                   VarNode& b, const bool shouldHold)
    : ArrayBoolXorNode(graph, std::vector<std::shared_ptr<VarNode>>{a.ptr(), b.ptr()},
                       shouldHold) {}

ArrayBoolXorNode::ArrayBoolXorNode(
    InvariantGraph& graph, std::vector<std::shared_ptr<VarNode>>&& inputs,
    VarNode& reified)
    : ViolationInvariantNode(graph, std::move(inputs), reified) {}

ArrayBoolXorNode::ArrayBoolXorNode(
    InvariantGraph& graph, std::vector<std::shared_ptr<VarNode>>&& inputs,
    const bool shouldHold)
    : ViolationInvariantNode(graph, std::move(inputs), shouldHold) {}

void ArrayBoolXorNode::init() {
  ViolationInvariantNode::init();
  assert(!isReified() || !reifiedViolationNode()->isIntVar());
  assert(std::ranges::none_of(staticInputVarNodes(),
                              [&](const std::shared_ptr<VarNode>& vNode) {
                                return vNode->isIntVar();
                              }));
}

void ArrayBoolXorNode::postConstraint() {
  if (isReified()) {
    return constraintSolver().array_bool_xor(
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodes()),
        reifiedViolationNode()->constraintVarId());
  }
  constraintSolver().array_bool_xor(
      toConstraintVarIds(invariantGraphConst(), staticInputVarNodes()),
      shouldHold());
}

void ArrayBoolXorNode::updateState() {
  ViolationInvariantNode::updateState();
  if (!isReified()) {
    const size_t numFixedTrue = std::ranges::count_if(
        staticInputVarNodes(), [&](const std::shared_ptr<VarNode>& vNode) {
          return vNode->isFixed() &&
                 vNode->inDomain(true);
        });
    const size_t numFixedFalse = std::ranges::count_if(
        staticInputVarNodes(), [&](const std::shared_ptr<VarNode>& vNode) {
          return vNode->isFixed() &&
                 vNode->inDomain(false);
        });
    if (shouldHold() ? (numFixedTrue == 1 &&
                        numFixedFalse == staticInputVarNodes().size() - 1)
                     : (numFixedTrue > 1 ||
                        numFixedFalse == staticInputVarNodes().size())) {
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  }

  std::vector<std::shared_ptr<VarNode>> varsToRemove;
  varsToRemove.reserve(staticInputVarNodes().size());
  for (const auto& vNode : staticInputVarNodes()) {
    if (vNode->isFixed()) {
      if (vNode->inDomain(bool{true})) {
        _containsFixedTrue = true;
      }
      varsToRemove.emplace_back(vNode);
    }
  }
  for (const auto& vNode : varsToRemove) {
    removeStaticInputVarNode(*vNode);
  }

  if (!_containsFixedTrue.has_value() && staticInputVarNodes().size() == 1 &&
      !varsToRemove.empty()) {
    _containsFixedTrue = false;
  }

  if (staticInputVarNodes().empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayBoolXorNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }
  if (isReified() && staticInputVarNodes().size() == 1) {
    assert(_containsFixedTrue.has_value());
    return true;
  }
  if (staticInputVarNodes().size() == 2) {
    return true;
  }
  return false;
}

bool ArrayBoolXorNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (staticInputVarNodes().size() == 1) {
    assert(isReified());
    assert(_containsFixedTrue.has_value());
    if (_containsFixedTrue.has_value() && *_containsFixedTrue) {
      invariantGraph().addInvariantNode(std::make_shared<BoolNotNode>(
          invariantGraph(), staticInputVarNode(0),
          outputVarNode(0)));
    } else {
      invariantGraph().replaceVarNode(*reifiedViolationNode(),
                                      staticInputVarNode(0));
    }
    return true;
  }
  assert(staticInputVarNodes().size() == 2);
  if (isReified()) {
    invariantGraph().addInvariantNode(std::make_shared<BoolRelNode>(
        invariantGraph(), staticInputVarNode(0),
        RelationType::REL_TYPE_NE, *staticInputVarNodes().back(),
        *reifiedViolationNode()));
  } else {
    invariantGraph().addInvariantNode(std::make_shared<BoolRelNode>(
        invariantGraph(), staticInputVarNode(0),
        RelationType::REL_TYPE_NE, *staticInputVarNodes().back(), shouldHold()));
  }
  return true;
}

void ArrayBoolXorNode::registerOutputVars(propagation::SolverBase& solver,
                                          SolverMapping& mapping) const {
  if (staticInputVarNodes().size() > 1 &&
      violationVarId(mapping) == propagation::NULL_ID) {
    if (staticInputVarNodes().size() == 2) {
      assert(isReified() || shouldHold());
      registerViolation(solver, mapping);
      return;
    }
    mapping.setIntermediateId(ptrConst(), solver.makeIntVar(0, 0, 0));
    if (shouldHold()) {
      setViolationVarId(solver.makeIntView<propagation::EqualConst>(
                            solver, mapping.intermediateId(ptrConst()), 1),
                        mapping);
    } else {
      assert(!isReified() && staticInputVarNodes().size() > 2);
      setViolationVarId(solver.makeIntView<propagation::NotEqualConst>(
                            solver, mapping.intermediateId(ptrConst()), 1),
                        mapping);
    }
  }
  assert(std::ranges::all_of(
      outputVarNodes(), [&](const std::shared_ptr<VarNode>& vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void ArrayBoolXorNode::registerNode(propagation::SolverBase& solver,
                                    SolverMapping& mapping) const {
  if (staticInputVarNodes().size() <= 1) {
    return;
  }
  assert(violationVarId(mapping) != propagation::NULL_ID);
  assert(staticInputVarNodes().size() == 2 ||
         mapping.intermediateId(ptrConst()) != propagation::NULL_ID);
  assert(staticInputVarNodes().size() == 2
             ? violationVarId(mapping).isVar()
             : mapping.intermediateId(ptrConst()).isVar());

  std::vector<propagation::VarViewId> inputNodeIds;
  std::ranges::transform(
      staticInputVarNodes(), std::back_inserter(inputNodeIds),
      [&](const auto& node) { return mapping.solverId(node); });
  if (staticInputVarNodes().size() == 2) {
    assert(isReified() || shouldHold());
    assert(mapping.intermediateId(ptrConst()) == propagation::NULL_ID);
    solver.makeInvariant<propagation::BoolXor>(solver, violationVarId(mapping),
                                               inputNodeIds.front(),
                                               inputNodeIds.back());
    return;
  }

  solver.makeInvariant<propagation::BoolLinear>(
      solver, mapping.intermediateId(ptrConst()), std::move(inputNodeIds));
}

std::string ArrayBoolXorNode::dotLangIdentifier() const {
  return "array_bool_xor";
}

}  // namespace atlantis::invariantgraph
