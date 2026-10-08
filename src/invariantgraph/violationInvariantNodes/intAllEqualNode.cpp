#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"

#include <algorithm>
#include <limits>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/allDifferentNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intRelNode.hpp"
#include "atlantis/propagation/invariants/countConst.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"
#include "atlantis/propagation/violationInvariants/equal.hpp"
#include "atlantis/propagation/violationInvariants/notEqual.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

IntAllEqualNode::IntAllEqualNode(InvariantGraph& graph, VarNode& a, VarNode& b,
                                 VarNode& r, const bool breaksCycle)
    : IntAllEqualNode(graph,
                      std::vector<std::shared_ptr<VarNode>>{a.ptr(), b.ptr()},
                      r, breaksCycle) {}

IntAllEqualNode::IntAllEqualNode(InvariantGraph& graph, VarNode& a, VarNode& b,
                                 const bool shouldHold, const bool breaksCycle)
    : IntAllEqualNode(graph,
                      std::vector<std::shared_ptr<VarNode>>{a.ptr(), b.ptr()},
                      shouldHold, breaksCycle) {}

IntAllEqualNode::IntAllEqualNode(InvariantGraph& graph,
                                 std::vector<std::shared_ptr<VarNode>>&& vars,
                                 VarNode& r, const bool breaksCycle)
    : ViolationInvariantNode(graph, std::move(vars), r),
      _breaksCycle(breaksCycle) {}

IntAllEqualNode::IntAllEqualNode(InvariantGraph& graph,
                                 std::vector<std::shared_ptr<VarNode>>&& vars,
                                 const bool shouldHold, const bool breaksCycle)
    : ViolationInvariantNode(graph, std::move(vars), shouldHold),
      _breaksCycle(breaksCycle) {}

void IntAllEqualNode::init() {
  ViolationInvariantNode::init();
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNode()).isIntVar());
  assert(std::ranges::all_of(staticInputVarNodes().begin(),
                             staticInputVarNodes().end(),
                             [&](VarNode& vId) { return vId.isIntVar(); }));
}

void IntAllEqualNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (staticInputVarNodes().size() == 2) {
    if (isReified()) {
      return constraintSolver().int_eq_reif(
          staticInputVarNodes().front().constraintVarId(),
          staticInputVarNodes().at(1).constraintVarId(),
          reifiedVarNodeConst().constraintVarId());
    }
    return constraintSolver().int_eq(
        staticInputVarNodes().front().constraintVarId(),
        staticInputVarNodes().at(1).constraintVarId(), shouldHold());
  }
  if (isReified()) {
    return constraintSolver().fzn_all_equal_int_reif(
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodes()),
        reifiedVarNodeConst().constraintVarId());
  }
  return constraintSolver().fzn_all_equal_int(
      toConstraintVarIds(invariantGraphConst(), staticInputVarNodes()),
      shouldHold());
}

void IntAllEqualNode::updateState() {
  ViolationInvariantNode::updateState();
  if (isReified()) {
    return;
  }
  if (shouldHold()) {
    const bool anyFixed =
        staticInputVarNodes().empty() ||
        std::ranges::any_of(staticInputVarNodes(),
                            [&](const std::shared_ptr<VarNode>& vId) {
                              return varNodeConst(vId).isFixed();
                            });
    if (anyFixed) {
      assert(std::ranges::all_of(
          staticInputVarNodes(), [&](const std::shared_ptr<VarNode>& vId) {
            return varNodeConst(vId).isFixed() &&
                   varNodeConst(vId).lowerBound() ==
                       staticInputVarNodes().front().lowerBound();
          }));
      setState(InvariantNodeState::SUBSUMED);
    }
    return;
  }

  std::vector<std::shared_ptr<VarNode>> varsToRemove;
  varsToRemove.reserve(staticInputVarNodes().size());

  for (const auto vId : staticInputVarNodes()) {
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
  if (staticInputVarNodes().size() == 1) {
    if (isReified()) {
      if (!_boundVal.has_value()) {
        setState(InvariantNodeState::SUBSUMED);
        return;
      }
    } else {
      assert(!shouldHold());
      auto& vNode = staticInputVarNodes().front();
      if (_boundVal.has_value() && vNode.lowerBound() <= *_boundVal &&
          *_boundVal <= vNode.upperBound()) {
        vNode.tightenDomainType(vNode.constDomain()->isInterval()
                                    ? DomainType::DOM_RANGE
                                    : DomainType::DOM_DOMAIN);
      }
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  }
  if (staticInputVarNodes().empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntAllEqualNode::canBeReplaced() const {
  if (isReified() && staticInputVarNodes().size() == 1 &&
      _boundVal.has_value()) {
    return true;
  }
  if (state() != InvariantNodeState::ACTIVE || _breaksCycle) {
    return false;
  }
  return !isReified() && (shouldHold() || (staticInputVarNodes().size() == 2 &&
                                           !_boundVal.has_value()));
}

bool IntAllEqualNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (isReified() && staticInputVarNodes().size() == 1 &&
      _boundVal.has_value()) {
    if (*_boundVal) {
      invariantGraph().replaceVarNode(reifiedViolationNode(),
                                      staticInputVarNodes().front());
    } else {
      invariantGraph().addInvariantNode(std::make_shared<BoolNotNode>(
          invariantGraph(), staticInputVarNodes().front(),
          reifiedViolationNode()));
    }
    return true;
  }
  if (!isReified() && shouldHold()) {
    assert(!_breaksCycle);
    const std::shared_ptr<VarNode>& frontVar = staticInputVarNodes().front();
    for (size_t i = 1; i < staticInputVarNodes().size(); ++i) {
      invariantGraph().replaceVarNode(staticInputVarNodes().at(i), frontVar);
    }
    return true;
  }
  assert(staticInputVarNodes().size() == 2);
  assert(!_boundVal.has_value());
  assert(!isReified());
  assert(!shouldHold());
  invariantGraph().addInvariantNode(std::make_shared<IntRelNode>(
      invariantGraph(), staticInputVarNodes().front(),
      RelationType::REL_TYPE_NE, staticInputVarNodes().back()));
  return true;
}

void IntAllEqualNode::registerOutputVars(propagation::SolverBase& solver,
                                         SolverMapping& mapping) const {
  assert(!staticInputVarNodes().empty());
  if (violationVarId(mapping) == propagation::NULL_ID) {
    if (_boundVal.has_value()) {
      if (staticInputVarNodes().size() == 1) {
        assert(isReified());
        assert(mapping.solverId(staticInputVarNodes().front()) !=
               propagation::NULL_ID);
        setViolationVarId(
            solver.makeIntView<propagation::EqualConst>(
                solver, mapping.solverId(staticInputVarNodes().front()),
                _boundVal.value()),
            mapping);
      } else if (mapping.intermediateId(TODO) == propagation::NULL_ID) {
        mapping.setIntermediateId(TODO, solver.makeIntVar(0, 0, 0));
        if (shouldHold()) {
          setViolationVarId(solver.makeIntView<propagation::EqualConst>(
                                solver, mapping.intermediateId(TODO),
                                staticInputVarNodes().size()),
                            mapping);
        } else {
          setViolationVarId(solver.makeIntView<propagation::NotEqualConst>(
                                solver, mapping.intermediateId(TODO),
                                staticInputVarNodes().size()),
                            mapping);
        }
      }
    } else if (staticInputVarNodes().size() == 2) {
      registerViolation(solver, mapping);
    } else if (mapping.intermediateId(TODO) == propagation::NULL_ID) {
      mapping.setIntermediateId(TODO, solver.makeIntVar(0, 0, 0));
      if (shouldHold()) {
        setViolationVarId(solver.makeIntView<propagation::EqualConst>(
                              solver, mapping.intermediateId(TODO),
                              staticInputVarNodes().size() - 1),
                          mapping);
      } else {
        assert(!isReified());
        setViolationVarId(solver.makeIntView<propagation::NotEqualConst>(
                              solver, mapping.intermediateId(TODO),
                              staticInputVarNodes().size() - 1),
                          mapping);
      }
    }
  }
  assert(std::ranges::all_of(outputVarNodes().begin(), outputVarNodes().end(),
                             [&](const std::shared_ptr<VarNode>& vId) {
                               return mapping.solverId(vId) !=
                                      propagation::NULL_ID;
                             }));
}

void IntAllEqualNode::registerNode(propagation::SolverBase& solver,
                                   SolverMapping& mapping) const {
  assert(violationVarId(mapping) != propagation::NULL_ID);

  if (_boundVal.has_value() && staticInputVarNodes().size() <= 1) {
    return;
  }

  assert(staticInputVarNodes().size() >= 2);

  std::vector<propagation::VarViewId> inputVarIds;
  inputVarIds.reserve(staticInputVarNodes().size());
  std::ranges::transform(staticInputVarNodes(), std::back_inserter(inputVarIds),
                         [&](const auto& id) { return mapping.solverId(id); });

  if (_boundVal.has_value()) {
    assert(mapping.intermediateId(TODO) != propagation::NULL_ID);
    assert(mapping.intermediateId(TODO).isVar());
    solver.makeInvariant<propagation::CountConst>(
        solver, mapping.intermediateId(TODO), _boundVal.value(),
        std::move(inputVarIds));
    return;
  }

  if (inputVarIds.size() == 2) {
    assert(violationVarId(mapping).isVar());
    assert(mapping.intermediateId(TODO) == propagation::NULL_ID);
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

  assert(mapping.intermediateId(TODO) != propagation::NULL_ID);
  assert(mapping.intermediateId(TODO).isVar());

  solver.makeViolationInvariant<propagation::AllDifferent>(
      solver, mapping.intermediateId(TODO), std::move(inputVarIds));
}

std::string IntAllEqualNode::dotLangIdentifier() const {
  return "int_all_equal";
}

}  // namespace atlantis::invariantgraph
