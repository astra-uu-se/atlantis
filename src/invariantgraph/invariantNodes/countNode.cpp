#include "atlantis/invariantgraph/invariantNodes/countNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/countImplicitNode.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/count.hpp"
#include "atlantis/propagation/invariants/countConst.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/ifThenElseConst.hpp"

namespace atlantis::invariantgraph {

VarNodeId CountNode::needle() const {
  return _fixedNeedle.has_value() ? NULL_NODE_ID
                                  : staticInputVarNodeIds()[needleIndex()];
  ;
}

size_t CountNode::needleIndex() const { return numInputVars(); }

size_t CountNode::numInputVars() const {
  return staticInputVarNodeIds().size() - (_fixedNeedle.has_value() ? 0 : 1);
}

CountNode::CountNode(InvariantGraph& graph, const VarNodeId count,
                     std::vector<VarNodeId>&& vars, const Int needle,
                     const Int countOffset)
    : InvariantNode(graph, std::vector<VarNodeId>{count}, std::move(vars)),
      _fixedNeedle(needle),
      _countOffset(countOffset) {}

CountNode::CountNode(InvariantGraph& graph, const VarNodeId count,
                     std::vector<VarNodeId>&& vars, const VarNodeId needle,
                     const Int countOffset)
    : InvariantNode(graph, std::vector<VarNodeId>{count},
                    append(std::move(vars), needle)),
      _fixedNeedle(std::nullopt),
      _countOffset(countOffset) {}

void CountNode::init(const InvariantNodeId id) {
  InvariantNode::init(id);
  assert(invariantGraphConst()
             .varNodeConst(outputVarNodeIds().front())
             .isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
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
  const bool allSourceVars =
      std::ranges::all_of(staticInputVarNodeIds(), [&](const auto& id) {
        return invariantGraphConst().varNodeConst(id).definingNodes().empty();
      });
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
                            .varNodeConst(outputVarNodeIds().front())
                            .lowerBound() -
                        _countOffset;

  invariantGraph().addImplicitConstraintNode(
      std::make_shared<CountImplicitNode>(
          invariantGraph(), std::vector<VarNodeId>(staticInputVarNodeIds()),
          *_fixedNeedle, amount));
  return true;
}

void CountNode::registerOutputVars(propagation::SolverBase& solver,
                                   SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() == 1 && _fixedNeedle.has_value()) {
    mapping.setSolverId(
        outputVarNodeIds().front(),
        solver.makeIntView<propagation::IfThenElseConst>(
            solver, mapping.solverId(staticInputVarNodeIds().front()),
            _countOffset + 1, _countOffset, *_fixedNeedle));
  } else {
    makeSolverVar(outputVarNodeIds().front(), solver, mapping);
  }
  assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
    return mapping.solverId(vId) != propagation::NULL_ID;
  }));
}

void CountNode::registerNode(propagation::SolverBase& solver,
                             SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() <= 1 && _fixedNeedle.has_value()) {
    return;
  }
  assert(mapping.solverId(outputVarNodeIds().front()) != propagation::NULL_ID);
  assert(mapping.intermediateId(id()) == propagation::NULL_ID
             ? mapping.solverId(outputVarNodeIds().front()).isVar()
             : mapping.solverId(outputVarNodeIds().front()).isView());
  assert(mapping.intermediateId(id()) == propagation::NULL_ID ||
         mapping.intermediateId(id()).isVar());

  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(numInputVars());

  for (size_t i = 0; i < numInputVars(); ++i) {
    solverVars.emplace_back(mapping.solverId(staticInputVarNodeIds()[i]));
  }

  if (_fixedNeedle.has_value()) {
    solver.makeInvariant<propagation::CountConst>(
        solver, mapping.solverId(outputVarNodeIds().front()), *_fixedNeedle,
        std::move(solverVars), _countOffset);
    return;
  }
  assert(_countOffset == 0);
  solver.makeInvariant<propagation::Count>(
      solver, mapping.solverId(outputVarNodeIds().front()),
      mapping.solverId(needle()), std::move(solverVars));
}

std::string CountNode::dotLangIdentifier() const {
  return _fixedNeedle.has_value()
             ? ("int_count " + std::to_string(*_fixedNeedle))
             : "var_int_count";
}

}  // namespace atlantis::invariantgraph
