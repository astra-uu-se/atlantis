#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"

#include <algorithm>
#include <limits>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/allDifferentNode.hpp"
#include "atlantis/propagation/invariants/countConst.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"
#include "atlantis/propagation/violationInvariants/equal.hpp"
#include "atlantis/propagation/violationInvariants/notEqual.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

IntAllEqualNode::IntAllEqualNode(InvariantGraph& graph, const VarNodeId a,
                                 const VarNodeId b, const VarNodeId r, const bool breaksCycle)
    : IntAllEqualNode(graph, std::vector<VarNodeId>{a, b}, r, breaksCycle) {}

IntAllEqualNode::IntAllEqualNode(InvariantGraph& graph, const VarNodeId a,
                                 const VarNodeId b, const bool shouldHold, const bool breaksCycle)
    : IntAllEqualNode(graph, std::vector<VarNodeId>{a, b}, shouldHold,
                      breaksCycle) {}

IntAllEqualNode::IntAllEqualNode(InvariantGraph& graph,
                                 std::vector<VarNodeId>&& vars, const VarNodeId r,
                                 const bool breaksCycle)
    : ViolationInvariantNode(graph, std::move(vars), r),
      _breaksCycle(breaksCycle) {}

IntAllEqualNode::IntAllEqualNode(InvariantGraph& graph,
                                 std::vector<VarNodeId>&& vars, const bool shouldHold,
                                 const bool breaksCycle)
    : ViolationInvariantNode(graph, std::move(vars), shouldHold),
      _breaksCycle(breaksCycle) {}

void IntAllEqualNode::init(const InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void IntAllEqualNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (staticInputVarNodeIds().size() == 2) {
    if (isReified()) {
      return constraintSolver().int_eq_reif(staticInputVarNodeConst(0).constraintVarId(), staticInputVarNodeConst(1).constraintVarId(), reifiedVarNodeConst().constraintVarId());
    }
    return constraintSolver().int_eq(staticInputVarNodeConst(0).constraintVarId(), staticInputVarNodeConst(1).constraintVarId(), shouldHold());
  }
  if (isReified()) {
    return constraintSolver().fzn_all_equal_int_reif(toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()), reifiedVarNodeConst().constraintVarId());
  }
  return constraintSolver().fzn_all_equal_int(toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()), shouldHold());
}

void IntAllEqualNode::updateState() {
  ViolationInvariantNode::updateState();
  if (isReified()) {
    return;
  }
  if (shouldHold()) {
    const bool anyFixed = staticInputVarNodeIds().empty() || std::ranges::any_of(staticInputVarNodeIds(), [&](const VarNodeId vId) {
      return varNodeConst(vId).isFixed();
    });
    if (anyFixed) {
      assert(std::ranges::all_of(staticInputVarNodeIds(), [&](const VarNodeId vId) {
      return varNodeConst(vId).isFixed() && varNodeConst(vId).lowerBound() == staticInputVarNodeConst(0).lowerBound();
    }));
      setState(InvariantNodeState::SUBSUMED);
    }
    return;
  }

  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());

  for (const auto vId : staticInputVarNodeIds()) {
    if (!varNodeConst(vId).isFixed()) {
      continue;
    }
    if (!_boundVal.has_value()) {
      _boundVal = varNodeConst(vId).lowerBound();
    } else if (*_boundVal != varNodeConst(vId).lowerBound()) {
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
    varsToRemove.emplace_back(vId);
  }
  for (const auto vId : varsToRemove) {
    removeStaticInputVarNode(vId);
  }
  assert(staticInputVarNodeIds().size() != 1);
  if (staticInputVarNodeIds().empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntAllEqualNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE || _breaksCycle) {
    return false;
  }
  return !isReified() &&
         (shouldHold() ||
          (staticInputVarNodeIds().size() <= 2 && !_boundVal.has_value()));
}

bool IntAllEqualNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (!isReified() && shouldHold()) {
    assert(!_breaksCycle);
    const VarNodeId frontVarId = staticInputVarNodeIds().front();
    for (size_t i = 1; i < staticInputVarNodeIds().size(); ++i) {
      invariantGraph().replaceVarNode(staticInputVarNodeIds().at(i),
                                      frontVarId);
    }
    return true;
  }
  assert(staticInputVarNodeIds().size() <= 2);
  assert(!_boundVal.has_value());
  invariantGraph().addInvariantNode(std::make_shared<AllDifferentNode>(
      invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()}));
  return true;
}

void IntAllEqualNode::registerOutputVars(propagation::SolverBase& solver,
                                         SolverMapping& mapping) const {
  assert(!staticInputVarNodeIds().empty());
  if (violationVarId(mapping) == propagation::NULL_ID) {
    if (_boundVal.has_value()) {
      if (staticInputVarNodeIds().size() == 1) {
        assert(isReified());
        assert(mapping.solverId(staticInputVarNodeIds().front()) !=
               propagation::NULL_ID);
        setViolationVarId(
            solver.makeIntView<propagation::EqualConst>(
                solver, mapping.solverId(staticInputVarNodeIds().front()),
                _boundVal.value()),
            mapping);
      } else if (mapping.intermediateId(id()) == propagation::NULL_ID) {
        mapping.setIntermediateId(id(), solver.makeIntVar(0, 0, 0));
        if (shouldHold()) {
          setViolationVarId(solver.makeIntView<propagation::EqualConst>(
                                solver, mapping.intermediateId(id()),
                                staticInputVarNodeIds().size()),
                            mapping);
        } else {
          setViolationVarId(solver.makeIntView<propagation::NotEqualConst>(
                                solver, mapping.intermediateId(id()),
                                staticInputVarNodeIds().size()),
                            mapping);
        }
      }
    } else if (staticInputVarNodeIds().size() == 2) {
      registerViolation(solver, mapping);
    } else if (mapping.intermediateId(id()) == propagation::NULL_ID) {
      mapping.setIntermediateId(id(), solver.makeIntVar(0, 0, 0));
      if (shouldHold()) {
        setViolationVarId(solver.makeIntView<propagation::EqualConst>(
                              solver, mapping.intermediateId(id()),
                              staticInputVarNodeIds().size() - 1),
                          mapping);
      } else {
        assert(!isReified());
        setViolationVarId(solver.makeIntView<propagation::NotEqualConst>(
                              solver, mapping.intermediateId(id()),
                              staticInputVarNodeIds().size() - 1),
                          mapping);
      }
    }
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void IntAllEqualNode::registerNode(propagation::SolverBase& solver,
                                   SolverMapping& mapping) const {
  assert(violationVarId(mapping) != propagation::NULL_ID);

  if (_boundVal.has_value() && staticInputVarNodeIds().size() <= 1) {
    return;
  }

  assert(staticInputVarNodeIds().size() >= 2);

  std::vector<propagation::VarViewId> inputVarIds;
  inputVarIds.reserve(staticInputVarNodeIds().size());
  std::ranges::transform(staticInputVarNodeIds(),
                         std::back_inserter(inputVarIds),
                         [&](const auto& id) { return mapping.solverId(id); });

  if (_boundVal.has_value()) {
    assert(mapping.intermediateId(id()) != propagation::NULL_ID);
    assert(mapping.intermediateId(id()).isVar());
    solver.makeInvariant<propagation::CountConst>(
        solver, mapping.intermediateId(id()), _boundVal.value(),
        std::move(inputVarIds));
    return;
  }

  if (inputVarIds.size() == 2) {
    assert(violationVarId(mapping).isVar());
    assert(mapping.intermediateId(id()) == propagation::NULL_ID);
    if (shouldHold()) {
      solver.makeViolationInvariant<propagation::Equal>(
          solver, violationVarId(mapping), inputVarIds.front(),
          inputVarIds.back());
    } else {
      solver.makeViolationInvariant<propagation::NotEqual>(
          solver, violationVarId(mapping), inputVarIds.front(),
          inputVarIds.back());
    }
    return;
  }

  assert(mapping.intermediateId(id()) != propagation::NULL_ID);
  assert(mapping.intermediateId(id()).isVar());

  solver.makeViolationInvariant<propagation::AllDifferent>(
      solver, mapping.intermediateId(id()), std::move(inputVarIds));
}

std::string IntAllEqualNode::dotLangIdentifier() const {
  return "int_all_equal";
}

}  // namespace atlantis::invariantgraph
