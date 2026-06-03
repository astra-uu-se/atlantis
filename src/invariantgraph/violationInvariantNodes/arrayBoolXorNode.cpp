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
#include "atlantis/propagation/invariants/boolLinear.hpp"
#include "atlantis/propagation/invariants/boolXor.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

ArrayBoolXorNode::ArrayBoolXorNode(InvariantGraph& graph, const VarNodeId a,
                                   const VarNodeId b, const VarNodeId reified)
    : ArrayBoolXorNode(graph, std::vector<VarNodeId>{a, b}, reified) {}

ArrayBoolXorNode::ArrayBoolXorNode(InvariantGraph& graph, const VarNodeId a,
                                   const VarNodeId b, const bool shouldHold)
    : ArrayBoolXorNode(graph, std::vector<VarNodeId>{a, b}, shouldHold) {}

ArrayBoolXorNode::ArrayBoolXorNode(InvariantGraph& graph,
                                   std::vector<VarNodeId>&& inputs,
                                   const VarNodeId reified)
    : ViolationInvariantNode(graph, std::move(inputs), reified) {}

ArrayBoolXorNode::ArrayBoolXorNode(InvariantGraph& graph,
                                   std::vector<VarNodeId>&& inputs,
                                   const bool shouldHold)
    : ViolationInvariantNode(graph, std::move(inputs), shouldHold) {}

void ArrayBoolXorNode::init(const InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(!isReified() || !reifiedVarNodeConst().isIntVar());
  assert(std::ranges::none_of(
      staticInputVarNodeIds(),
      [&](const VarNodeId vId) { return varNodeConst(vId).isIntVar(); }));
}

void ArrayBoolXorNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (staticInputVarNodeIds().size() < 2) {
    return;
  }
  if (staticInputVarNodeIds().size() == 2) {
    if (isReified()) {
      return constraintSolver().bool_xor_reif(
          staticInputVarNodeConst(0).constraintVarId(),
          staticInputVarNodeConst(1).constraintVarId(),
          reifiedVarNodeConst().constraintVarId());
    }
    return constraintSolver().bool_xor(
        staticInputVarNodeConst(0).constraintVarId(),
        staticInputVarNodeConst(1).constraintVarId(), shouldHold());
  }
  if (isReified()) {
    constraintSolver().array_bool_xor(
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
        reifiedVarNodeConst().constraintVarId());
  } else {
    constraintSolver().array_bool_xor(
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
        shouldHold());
  }
}

void ArrayBoolXorNode::updateState() {
  ViolationInvariantNode::updateState();
  if (!isReified()) {
    const size_t numFixedTrue = std::ranges::count_if(
        staticInputVarNodeIds(), [&](const VarNodeId vId) {
          return varNodeConst(vId).isFixed() &&
                 varNodeConst(vId).inDomain(true);
        });
    const size_t numFixedFalse = std::ranges::count_if(
        staticInputVarNodeIds(), [&](const VarNodeId vId) {
          return varNodeConst(vId).isFixed() &&
                 varNodeConst(vId).inDomain(false);
        });
    if (shouldHold() ? (numFixedTrue == 1 &&
                        numFixedFalse == staticInputVarNodeIds().size() - 1)
                     : (numFixedTrue > 1 ||
                        numFixedFalse == staticInputVarNodeIds().size())) {
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  }

  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());
  for (const auto& id : staticInputVarNodeIds()) {
    if (varNodeConst(id).isFixed()) {
      varsToRemove.emplace_back(id);
    }
  }
  for (const auto& id : varsToRemove) {
    removeStaticInputVarNode(id);
  }

  assert(!staticInputVarNodeIds().empty());

  if (staticInputVarNodeIds().empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayBoolXorNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }
  if (isReified() && staticInputVarNodeIds().size() == 1) {
    return true;
  }
  if (staticInputVarNodeIds().size() == 2) {
    if (!isReified()) {
      return !shouldHold();
    }
    return std::ranges::count_if(
               staticInputVarNodeIds(), [&](const VarNodeId vId) {
                 return varNodeConst(vId).isFixed() &&
                        varNodeConst(vId).inDomain(bool{true});
               }) == 1;
  }
  return false;
}

bool ArrayBoolXorNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1) {
    assert(isReified());
    invariantGraph().replaceVarNode(reifiedViolationNodeId(),
                                    staticInputVarNodeIds().front());
  }
  if (staticInputVarNodeIds().size() == 2) {
    if (!isReified() && !shouldHold()) {
      invariantGraph().addInvariantNode(std::make_shared<BoolAllEqualNode>(
          invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()},
          true));
    } else {
      assert(isReified());
      assert(std::ranges::count_if(
                 staticInputVarNodeIds(), [&](const VarNodeId vId) {
                   return varNodeConst(vId).isFixed() &&
                          varNodeConst(vId).inDomain(bool{true});
                 }) == 1);
      const VarNodeId vId =
          varNodeConst(staticInputVarNodeIds().front()).isFixed() &&
                  varNodeConst(staticInputVarNodeIds().front())
                      .inDomain(bool{true})
              ? staticInputVarNodeIds().back()
              : staticInputVarNodeIds().front();
      invariantGraph().addInvariantNode(std::make_shared<BoolNotNode>(
          invariantGraph(), vId, reifiedViolationNodeId()));
    }
  }
  return true;
}

void ArrayBoolXorNode::registerOutputVars(propagation::SolverBase& solver,
                                          SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() > 1 &&
      violationVarId(mapping) == propagation::NULL_ID) {
    if (staticInputVarNodeIds().size() == 2) {
      assert(isReified() || shouldHold());
      registerViolation(solver, mapping);
      return;
    }
    mapping.setIntermediateId(id(), solver.makeIntVar(0, 0, 0));
    if (shouldHold()) {
      setViolationVarId(solver.makeIntView<propagation::EqualConst>(
                            solver, mapping.intermediateId(id()), 1),
                        mapping);
    } else {
      assert(!isReified() && staticInputVarNodeIds().size() > 2);
      setViolationVarId(solver.makeIntView<propagation::NotEqualConst>(
                            solver, mapping.intermediateId(id()), 1),
                        mapping);
    }
  }
  assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
    return mapping.solverId(vId) != propagation::NULL_ID;
  }));
}

void ArrayBoolXorNode::registerNode(propagation::SolverBase& solver,
                                    SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  assert(violationVarId(mapping) != propagation::NULL_ID);
  assert(staticInputVarNodeIds().size() == 2 ||
         mapping.intermediateId(id()) != propagation::NULL_ID);
  assert(staticInputVarNodeIds().size() == 2
             ? violationVarId(mapping).isVar()
             : mapping.intermediateId(id()).isVar());

  std::vector<propagation::VarViewId> inputNodeIds;
  std::ranges::transform(
      staticInputVarNodeIds(), std::back_inserter(inputNodeIds),
      [&](const auto& node) { return mapping.solverId(node); });
  if (staticInputVarNodeIds().size() == 2) {
    assert(isReified() || shouldHold());
    assert(mapping.intermediateId(id()) == propagation::NULL_ID);
    solver.makeInvariant<propagation::BoolXor>(solver, violationVarId(mapping),
                                               inputNodeIds.front(),
                                               inputNodeIds.back());
    return;
  }

  solver.makeInvariant<propagation::BoolLinear>(
      solver, mapping.intermediateId(id()), std::move(inputNodeIds));
}

std::string ArrayBoolXorNode::dotLangIdentifier() const {
  return "array_bool_xor";
}

}  // namespace atlantis::invariantgraph
