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

std::shared_ptr<VarNode> CountNode::needle() const {
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
    : InvariantNode(graph, std::vector<std::shared_ptr<VarNode>>{count.ptr()},
                    std::move(vars)),
      _fixedNeedle(needle),
      _countOffset(countOffset) {}

CountNode::CountNode(InvariantGraph& graph, VarNode& count,
                     std::vector<std::shared_ptr<VarNode>>&& vars,
                     VarNode& needle, const Int countOffset)
    : InvariantNode(graph, std::vector<std::shared_ptr<VarNode>>{count.ptr()},
                    append(std::move(vars), needle)),
      _fixedNeedle(std::nullopt),
      _countOffset(countOffset) {}

void CountNode::init() {
  InvariantNode::init();
  assert(outputVarNode(0).isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodes(),
      [&](const std::shared_ptr<VarNode>& vNode) { return vNode->isIntVar(); }));
}

void CountNode::postConstraint() {
  InvariantNode::postConstraint();
  std::vector<ConstraintVarId> inputs(numInputVars(),
                                      ConstraintVarId{NULL_NODE_ID});
  for (size_t i = 0; i < inputs.size(); ++i) {
    inputs[i] = staticInputVarNode(i).constraintVarId();
  }
  if (_fixedNeedle.has_value()) {
    return constraintSolver().fzn_count(
        outputVarNode(0).constraintVarId(), RelationType::REL_TYPE_EQ,
        inputs, *_fixedNeedle, true);
  }
  return constraintSolver().fzn_count(
      outputVarNode(0).constraintVarId(), RelationType::REL_TYPE_EQ,
      inputs, needle()->constraintVarId(), true);
}

void CountNode::updateState() {
  InvariantNode::updateState();
  if (!_fixedNeedle.has_value() && needle()->isFixed()) {
    // updating _fixedNeedle modified indices
    const Int n = needle()->lowerBound();
    removeStaticInputAtIndex(needleIndex());
    _fixedNeedle = n;
  }
  std::vector<Int> indicesToRemove;
  indicesToRemove.reserve(numInputVars());
  for (Int i = static_cast<Int>(numInputVars()) - 1; i >= 0; --i) {
    if (_fixedNeedle.has_value()) {
      if (staticInputVarNode(i).isFixed()) {
        _countOffset +=
            (*_fixedNeedle == staticInputVarNode(i).lowerBound() ? 1 : 0);
        indicesToRemove.emplace_back(i);
      } else if (!staticInputVarNode(i).inDomain(*_fixedNeedle)) {
        indicesToRemove.emplace_back(i);
      }
    } else if (staticInputVarNode(i).constDomain()->isDisjoint(
                   *needle()->constDomain())) {
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
  if (outputVarNode(0).isFixed() && _fixedNeedle.has_value() &&
      _countOffset + outputVarNode(0).lowerBound() <= 0) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool CountNode::constrainsOutput(const VarNode&) const {
  return !outputVarNode(0).constDomain()->contains(
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
  if (!outputVarNode(0).isFixed()) {
    return false;
  }
  if (outputVarNode(0).lowerBound() + _countOffset <= 0) {
    return false;
  }
  const bool allSourceVars = std::ranges::all_of(
      staticInputVarNodes(),
      [&](const std::shared_ptr<VarNode>& vNode) { return vNode->definingNodes().empty(); });
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

  const size_t amount = outputVarNode(0).lowerBound() -
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
        outputVarNode(0),
        solver.makeIntView<propagation::IfThenElseConst>(
            solver, mapping.solverId(staticInputVarNode(0)),
            _countOffset + 1, _countOffset, *_fixedNeedle));
  } else {
    makeSolverVar(outputVarNode(0), solver, mapping);
  }
  assert(std::ranges::all_of(
      outputVarNodes(), [&](const std::shared_ptr<VarNode>& vNode) {
        return mapping.solverId(vNode) != propagation::NULL_ID;
      }));
}

void CountNode::registerNode(propagation::SolverBase& solver,
                             SolverMapping& mapping) const {
  if (staticInputVarNodes().size() <= 1 && _fixedNeedle.has_value()) {
    return;
  }
  assert(mapping.solverId(outputVarNode(0)) != propagation::NULL_ID);
  assert(mapping.intermediateId(ptrConst()) == propagation::NULL_ID
             ? mapping.solverId(outputVarNode(0)).isVar()
             : mapping.solverId(outputVarNode(0)).isView());
  assert(mapping.intermediateId(ptrConst()) == propagation::NULL_ID ||
         mapping.intermediateId(ptrConst()).isVar());

  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(numInputVars());

  for (size_t i = 0; i < numInputVars(); ++i) {
    solverVars.emplace_back(mapping.solverId(staticInputVarNode(i)));
  }

  if (_fixedNeedle.has_value()) {
    solver.makeInvariant<propagation::CountConst>(
        solver, mapping.solverId(outputVarNode(0)), *_fixedNeedle,
        std::move(solverVars), _countOffset);
    return;
  }
  assert(_countOffset == 0);
  solver.makeInvariant<propagation::Count>(
      solver, mapping.solverId(outputVarNode(0)),
      mapping.solverId(needle()), std::move(solverVars));
}

std::string CountNode::dotLangIdentifier() const {
  return _fixedNeedle.has_value()
             ? ("int_count " + std::to_string(*_fixedNeedle))
             : "var_int_count";
}

}  // namespace atlantis::invariantgraph
