#include "atlantis/invariantgraph/violationInvariantNodes/boolAllEqualNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolOrNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolXorNode.hpp"
#include "atlantis/propagation/invariants/boolXor.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/boolAllEqual.hpp"
#include "atlantis/propagation/violationInvariants/boolEqual.hpp"

namespace atlantis::invariantgraph {

BoolAllEqualNode::BoolAllEqualNode(InvariantGraph& graph, const VarNodeId a,
                                   const VarNodeId b, const VarNodeId r,
                                   const bool breaksCycle)
    : BoolAllEqualNode(graph, std::vector<VarNodeId>{a, b}, r, breaksCycle) {}

BoolAllEqualNode::BoolAllEqualNode(InvariantGraph& graph, const VarNodeId a,
                                   const VarNodeId b, const bool shouldHold,
                                   const bool breaksCycle)
    : BoolAllEqualNode(graph, std::vector<VarNodeId>{a, b}, shouldHold,
                       breaksCycle) {}

BoolAllEqualNode::BoolAllEqualNode(InvariantGraph& graph,
                                   std::vector<VarNodeId>&& vars,
                                   const VarNodeId r, const bool breaksCycle)
    : ViolationInvariantNode(graph, std::move(vars), r),
      _breaksCycle(breaksCycle) {}

BoolAllEqualNode::BoolAllEqualNode(InvariantGraph& graph,
                                   std::vector<VarNodeId>&& vars,
                                   const bool shouldHold,
                                   const bool breaksCycle)
    : ViolationInvariantNode(graph, std::move(vars), shouldHold),
      _breaksCycle(breaksCycle) {}

bool BoolAllEqualNode::isFixed() const { return _dom < 2; }

bool BoolAllEqualNode::inDomain(const bool val) const {
  return val ? holdsTrue() : holdsFalse();
}

bool BoolAllEqualNode::holdsTrue() const { return _dom > 0; }

bool BoolAllEqualNode::holdsFalse() const { return _dom != 1; }

void BoolAllEqualNode::fixToVal(const bool val) { _dom = val ? 1 : 0; }

void BoolAllEqualNode::init(const InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::ranges::none_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void BoolAllEqualNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (staticInputVarNodeIds().size() < 2) {
    return;
  }
  if (staticInputVarNodeIds().size() == 2) {
    if (isReified()) {
      constraintSolver().bool_eq_reif(staticInputVarNode(0).constraintVarId(),
                                      staticInputVarNode(1).constraintVarId(),
                                      reifiedVarNodeConst().constraintVarId());
    } else {
      constraintSolver().bool_eq(staticInputVarNode(0).constraintVarId(),
                                 staticInputVarNode(1).constraintVarId(),
                                 shouldHold());
    }
    return;
  }
  const std::vector<Int> coeffs(staticInputVarNodeIds().size(), 1);
  std::vector<ConstraintVarId> inputs(staticInputVarNodeIds().size(),
                                      ConstraintVarId{NULL_NODE_ID});
  for (size_t i = 0; i < staticInputVarNodeIds().size(); i++) {
    inputs[i] = staticInputVarNode(i).constraintVarId();
  }
  const Int rhs = static_cast<Int>(staticInputVarNodeIds().size());
  if (isReified()) {
    constraintSolver().bool_lin_eq_reif(
        coeffs, inputs, rhs, reifiedVarNodeConst().constraintVarId());
  } else {
    constraintSolver().bool_lin_eq(coeffs, inputs, rhs, shouldHold());
  }
}

void BoolAllEqualNode::updateState() {
  ViolationInvariantNode::updateState();

  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());

  for (const auto vId : staticInputVarNodeIds()) {
    VarNode& vNode = varNode(vId);
    if (vNode.isFixed()) {
      assert(!isReified());
      const bool val = vNode.inDomain(bool{true});
      if (inDomain(val)) {
        if (!isFixed()) {
          fixToVal(val);
        }
      }
      setState(InvariantNodeState::SUBSUMED);
      varsToRemove.emplace_back(vId);
    }
  }

  for (const auto vId : varsToRemove) {
    removeStaticInputVarNode(vId);
  }

  if (staticInputVarNodeIds().empty()) {
    assert(!isReified());
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool BoolAllEqualNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE || _breaksCycle) {
    return false;
  }
  if (isFixed()) {
    return true;
  }
  return !isReified() && (shouldHold() || staticInputVarNodeIds().size() == 2);
}

bool BoolAllEqualNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (isFixed()) {
    if (shouldHold()) {
      // node is reified
      if (holdsTrue()) {
        // all fixed vars takes value true
        if (isReified()) {
          // The reified var is true iff all unfixed vars takes value true
          invariantGraph().addInvariantNode(std::make_shared<ArrayBoolAndNode>(
              invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()},
              reifiedViolationNodeId()));
        } else {
          // The constraint holds iff all unfixed vars takes value true
          invariantGraph().addInvariantNode(std::make_shared<ArrayBoolAndNode>(
              invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()},
              true));
        }
      } else if (isReified()) {
        // The reified var is true iff all unfixed vars takes value false
        const VarNodeId invReif = invariantGraph().retrieveBoolVarNode();
        invariantGraph().addInvariantNode(std::make_shared<BoolNotNode>(
            invariantGraph(), reifiedViolationNodeId(), invReif));
        invariantGraph().addInvariantNode(std::make_shared<ArrayBoolOrNode>(
            invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()},
            invReif));
      } else {
        // The constraint holds iff all unfixed vars takes value false
        invariantGraph().addInvariantNode(std::make_shared<ArrayBoolOrNode>(
            invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()},
            false));
      }
    } else {
      assert(!isReified());
      if (holdsTrue()) {
        // The constraint holds iff any unfixed var takes value false
        invariantGraph().addInvariantNode(std::make_shared<ArrayBoolAndNode>(
            invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()},
            false));
      } else {
        // The constraint holds iff any unfixed var takes value true
        invariantGraph().addInvariantNode(std::make_shared<ArrayBoolOrNode>(
            invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()},
            true));
      }
    }
    return true;
  }
  assert(!isReified());
  if (shouldHold()) {
    assert(!_breaksCycle);
    const VarNodeId frontVarId = staticInputVarNodeIds().front();
    for (size_t i = 1; i < staticInputVarNodeIds().size(); ++i) {
      invariantGraph().replaceVarNode(staticInputVarNodeIds().at(i),
                                      frontVarId);
    }
    return true;
  }
  assert(staticInputVarNodeIds().size() == 2);
  assert(!isReified() && !shouldHold());
  invariantGraph().addInvariantNode(std::make_shared<ArrayBoolXorNode>(
      invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()}, true));
  return true;
}

void BoolAllEqualNode::registerOutputVars(propagation::SolverBase& solver,
                                          SolverMapping& mapping) const {
  if (violationVarId(mapping) == propagation::NULL_ID) {
    if (shouldHold() || staticInputVarNodeIds().size() == 2) {
      registerViolation(solver, mapping);
      assert(mapping.intermediateId(id()) == propagation::NULL_ID);
    } else if (!shouldHold()) {
      assert(!isReified());
      mapping.setIntermediateId(id(), solver.makeIntVar(0, 0, 0));
      setViolationVarId(solver.makeIntView<propagation::NotEqualConst>(
                            solver, mapping.intermediateId(id()), 0),
                        mapping);
    }
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void BoolAllEqualNode::registerNode(propagation::SolverBase& solver,
                                    SolverMapping& mapping) const {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  assert(violationVarId(mapping) != propagation::NULL_ID);

  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(staticInputVarNodeIds().size());
  std::ranges::transform(staticInputVarNodeIds(),
                         std::back_inserter(solverVars),
                         [&](const auto& id) { return mapping.solverId(id); });

  if (solverVars.size() == 2) {
    assert(mapping.intermediateId(id()) == propagation::NULL_ID);
    assert(violationVarId(mapping).isVar());
    if (shouldHold()) {
      solver.makeViolationInvariant<propagation::BoolEqual>(
          solver, violationVarId(mapping), solverVars.front(),
          solverVars.back());

    } else {
      solver.makeInvariant<propagation::BoolXor>(
          solver, violationVarId(mapping), solverVars.front(),
          solverVars.back());
    }
    return;
  }

  assert(shouldHold() !=
         (mapping.intermediateId(id()) != propagation::NULL_ID));
  assert(shouldHold() ? violationVarId(mapping).isVar()
                      : violationVarId(mapping).isView());

  solver.makeViolationInvariant<propagation::BoolAllEqual>(
      solver,
      mapping.intermediateId(id()) == propagation::NULL_ID
          ? violationVarId(mapping)
          : mapping.intermediateId(id()),
      std::move(solverVars));
}

std::string BoolAllEqualNode::dotLangIdentifier() const {
  return "bool_all_equal";
}

}  // namespace atlantis::invariantgraph
