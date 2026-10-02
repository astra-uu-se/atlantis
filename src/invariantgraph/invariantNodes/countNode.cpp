#include "atlantis/invariantgraph/invariantNodes/countNode.hpp"

#include <algorithm>
#include <utility>

#include "../implicitRanks.hpp"
#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/countImplicitNode.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/count.hpp"
#include "atlantis/propagation/invariants/countConst.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/ifThenElseConst.hpp"

namespace atlantis::invariantgraph {

const std::shared_ptr<VarNode>& CountNode::needle() const {
  return _fixedNeedle.has_value() ? std::shared_ptr<VarNode>{nullptr}
                                  : staticInputVarNodes()[needleIndex()];
}

size_t CountNode::needleIndex() const { return numInputVars(); }

size_t CountNode::numInputVars() const {
  return staticInputVarNodes().size() - (_fixedNeedle.has_value() ? 0 : 1);
}

CountNode::CountNode(InvariantGraph& graph, VarNode& count,
                     std::vector<std::shared_ptr<VarNode>>&& vars,
                     const Int needle, const Int countOffset)
    : InvariantNode(graph, std::vector<std::shared_ptr<VarNode>>{count},
                    std::move(vars)),
      _fixedNeedle(needle),
      _countOffset(countOffset) {}

CountNode::CountNode(InvariantGraph& graph, VarNode& count,
                     std::vector<std::shared_ptr<VarNode>>&& vars,
                     VarNode& needle, const Int countOffset)
    : InvariantNode(graph, std::vector<std::shared_ptr<VarNode>>{count},
                    append(std::move(vars), needle)),
      _fixedNeedle(std::nullopt),
      _countOffset(countOffset) {}

void CountNode::init() {
  InvariantNode::init();
  assert(
      invariantGraphConst().varNodeConst(outputVarNodes().front()).isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodes().begin(), staticInputVarNodes().end(),
      [&](const std::shared_ptr<VarNode>& vId) { return vId.isIntVar(); }));
}

void CountNode::postConstraint() {
  InvariantNode::postConstraint();
  std::vector<ConstraintVarId> inputs(numInputVars(),
                                      ConstraintVarId{NULL_NODE_ID});
  for (size_t i = 0; i < inputs.size(); ++i) {
    inputs[i] = staticInputVarNodeConst(i).constraintVarId();
  }
  if (_fixedNeedle.has_value()) {
    return constraintSolver().fzn_count(outputVarNodeConst(0).constraintVarId(),
                                        RelationType::REL_TYPE_EQ, inputs,
                                        *_fixedNeedle, true);
  }
  return constraintSolver().fzn_count(
      outputVarNodeConst(0).constraintVarId(), RelationType::REL_TYPE_EQ,
      inputs, varNodeConst(needle()).constraintVarId(), true);
}

void CountNode::updateState() {
  InvariantNode::updateState();
  if (!_fixedNeedle.has_value() && varNodeConst(needle()).isFixed()) {
    // updating _fixedNeedle modified indices
    const Int n = varNodeConst(needle()).lowerBound();
    removeStaticInputAtIndex(needleIndex());
    _fixedNeedle = n;
  }
  std::vector<Int> indicesToRemove;
  indicesToRemove.reserve(numInputVars());
  for (Int i = static_cast<Int>(numInputVars()) - 1; i >= 0; --i) {
    if (_fixedNeedle.has_value()) {
      if (staticInputVarNodeConst(i).isFixed()) {
        _countOffset +=
            (*_fixedNeedle == staticInputVarNodeConst(i).lowerBound() ? 1 : 0);
        indicesToRemove.emplace_back(i);
      } else if (!staticInputVarNodeConst(i).inDomain(*_fixedNeedle)) {
        indicesToRemove.emplace_back(i);
      }
    } else if (staticInputVarNodeConst(i).constDomain()->isDisjoint(
                   *varNodeConst(needle()).constDomain())) {
      indicesToRemove.emplace_back(i);
    }
  }
  for (const Int index : indicesToRemove) {
    removeStaticInputAtIndex(index);
  }
  if (numInputVars() == 0) {
    assert(outputVarNode(0).isFixed());
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (outputVarNodeConst(0).isFixed() && _fixedNeedle.has_value() &&
      _countOffset + outputVarNodeConst(0).lowerBound() <= 0) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool CountNode::constrainsOutput(VarNode&) const {
  return !outputVarNodeConst(0).constDomain()->contains(
      _countOffset, static_cast<Int>(staticInputVarNodes().size()));
}

std::pair<size_t, size_t> CountNode::implicitRank() const {
  return {rank::IMPLICIT_RANK_COUNT,
          staticInputVarNodes().size() + outputVarNodes().size()};
}

bool CountNode::canBeMadeImplicit() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }
  if (!_fixedNeedle.has_value()) {
    return false;
  }
  if (!outputVarNodeConst(0).isFixed()) {
    return false;
  }
  if (outputVarNodeConst(0).lowerBound() + _countOffset <= 0) {
    return false;
  }
  const bool allSourceVars = std::ranges::all_of(
      staticInputVarNodes(),
      [&](const auto& id) { return id.definingNodes().empty(); });
  if (!allSourceVars) {
    return false;
  }
  return true;
}

bool CountNode::makeImplicit() {
  if (!canBeMadeImplicit()) {
    return false;
  }

  assert(_fixedNeedle.has_value());

  const size_t amount = invariantGraphConst()
                            .varNodeConst(outputVarNodes().front())
                            .lowerBound() -
                        _countOffset;

  invariantGraph().addImplicitConstraintNode(
      std::make_shared<CountImplicitNode>(
          invariantGraph(),
          std::vector<std::shared_ptr<VarNode>>(staticInputVarNodes()),
          *_fixedNeedle, amount));
  return true;
}

void CountNode::registerOutputVars(propagation::SolverBase& solver,
                                   SolverMapping& mapping) const {
  if (staticInputVarNodes().size() == 1 && _fixedNeedle.has_value()) {
    mapping.setSolverId(
        outputVarNodes().front(),
        solver.makeIntView<propagation::IfThenElseConst>(
            solver, mapping.solverId(staticInputVarNodes().front()),
            _countOffset + 1, _countOffset, *_fixedNeedle));
  } else {
    makeSolverVar(outputVarNodes().front(), solver, mapping);
  }
  assert(std::ranges::all_of(
      outputVarNodes(), [&](const std::shared_ptr<VarNode>& vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void CountNode::registerNode(propagation::SolverBase& solver,
                             SolverMapping& mapping) const {
  if (staticInputVarNodes().size() <= 1 && _fixedNeedle.has_value()) {
    return;
  }
  assert(mapping.solverId(outputVarNodes().front()) != propagation::NULL_ID);
  assert(mapping.intermediateId(id()) == propagation::NULL_ID
             ? mapping.solverId(outputVarNodes().front()).isVar()
             : mapping.solverId(outputVarNodes().front()).isView());
  assert(mapping.intermediateId(id()) == propagation::NULL_ID ||
         mapping.intermediateId(id()).isVar());

  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(numInputVars());

  for (size_t i = 0; i < numInputVars(); ++i) {
    solverVars.emplace_back(mapping.solverId(staticInputVarNodes()[i]));
  }

  if (_fixedNeedle.has_value()) {
    solver.makeInvariant<propagation::CountConst>(
        solver, mapping.solverId(outputVarNodes().front()), *_fixedNeedle,
        std::move(solverVars), _countOffset);
    return;
  }
  assert(_countOffset == 0);
  solver.makeInvariant<propagation::Count>(
      solver, mapping.solverId(outputVarNodes().front()),
      mapping.solverId(needle()), std::move(solverVars));
}

std::string CountNode::dotLangIdentifier() const {
  return _fixedNeedle.has_value()
             ? ("int_count " + std::to_string(*_fixedNeedle))
             : "var_int_count";
}

}  // namespace atlantis::invariantgraph
