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

CountNode::CountNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
                     const Int needle, const VarNodeId count, const Int offset)
    : InvariantNode(graph, std::vector<VarNodeId>{count}, std::move(vars)),
      _fixedNeedle(needle),
      _offset(offset) {}

CountNode::CountNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
                     const VarNodeId needle, const VarNodeId count,
                     const Int offset)
    : InvariantNode(graph, std::vector<VarNodeId>{count},
                    append(std::move(vars), needle)),
      _fixedNeedle(std::nullopt),
      _offset(offset) {}

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
    return constraintSolver().fzn_count(
        inputs, *_fixedNeedle, RelationType::REL_TYPE_EQ,
        outputVarNodeConst(0).constraintVarId(), true);
  }
  return constraintSolver().fzn_count(
      inputs, varNodeConst(needle()).constraintVarId(),
      RelationType::REL_TYPE_EQ, outputVarNodeConst(0).constraintVarId(), true);
}

void CountNode::updateState() {
  InvariantNode::updateState();
  if (!_fixedNeedle.has_value() && varNodeConst(needle()).isFixed()) {
    _fixedNeedle = varNodeConst(needle()).lowerBound();
    removeStaticInputAtIndex(needleIndex());
  }
  std::vector<Int> indicesToRemove;
  indicesToRemove.reserve(numInputVars());
  for (Int i = static_cast<Int>(numInputVars()) - 1; i >= 0; --i) {
    if (_fixedNeedle.has_value() && staticInputVarNodeConst(i).isFixed()) {
      _offset +=
          (*_fixedNeedle == staticInputVarNodeConst(i).lowerBound() ? 1 : 0);
      indicesToRemove.emplace_back(i);
      continue;
    }
    if (!_fixedNeedle.has_value() &&
        staticInputVarNodeConst(i).constDomain()->isDisjoint(
            *varNodeConst(needle()).constDomain())) {
      indicesToRemove.emplace_back(i);
    }
  }
  for (const Int index : indicesToRemove) {
    removeStaticInputAtIndex(index);
  }
  if (staticInputVarNodeIds().empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool CountNode::canBeMadeImplicit() const {
  return state() == InvariantNodeState::ACTIVE && !isReified() &&
         !_fixedNeedle.has_value() &&
         std::ranges::all_of(staticInputVarNodeIds(),
                             [&](const auto& id) {
                               return invariantGraphConst()
                                   .varNodeConst(id)
                                   .definingNodes()
                                   .empty();
                             }) &&
         invariantGraphConst()
             .varNodeConst(outputVarNodeIds().front())
             .isFixed();
}

bool CountNode::makeImplicit() {
  if (!canBeMadeImplicit()) {
    return false;
  }

  assert(_fixedNeedle.has_value());

  const size_t amount = invariantGraphConst()
                            .varNodeConst(outputVarNodeIds().front())
                            .lowerBound() -
                        _offset;

  invariantGraph().addImplicitConstraintNode(
      std::make_shared<CountImplicitNode>(
          invariantGraph(), std::vector<VarNodeId>(staticInputVarNodeIds()),
          *_fixedNeedle, amount));
  return true;
}

void CountNode::registerOutputVars(propagation::SolverBase& solver,
                                   SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() == 1) {
    if (_fixedNeedle.has_value()) {
      mapping.setSolverId(
          outputVarNodeIds().front(),
          solver.makeIntView<propagation::IfThenElseConst>(
              solver, mapping.solverId(staticInputVarNodeIds().front()),
              _offset + 1, _offset, *_fixedNeedle));
    }
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
        std::move(solverVars), _offset);
    return;
  }
  assert(_offset == 0);
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
