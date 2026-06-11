#include "atlantis/invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"

#include <algorithm>
#include <limits>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/countRelNode.hpp"
#include "atlantis/propagation/invariants/max.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/intMaxView.hpp"

namespace atlantis::invariantgraph {

ArrayIntMaximumNode::ArrayIntMaximumNode(InvariantGraph& graph,
                                         const VarNodeId a, const VarNodeId b,
                                         const VarNodeId output)
    : ArrayIntMaximumNode(graph, std::vector<VarNodeId>{a, b}, output) {}

ArrayIntMaximumNode::ArrayIntMaximumNode(InvariantGraph& graph,
                                         std::vector<VarNodeId>&& vars,
                                         const VarNodeId output)
    : InvariantNode(graph, {output}, std::move(vars)),
      _lowerBound(std::numeric_limits<Int>::min()) {}

void ArrayIntMaximumNode::init(const InvariantNodeId id) {
  InvariantNode::init(id);
  assert(outputVarNode(0).isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void ArrayIntMaximumNode::postConstraint() {
  constraintSolver().array_int_maximum(
      toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
      outputVarNode(0).constraintVarId());
}

void ArrayIntMaximumNode::updateState() {
  InvariantNode::updateState();
  const auto duplicateIndices =
      duplicateVarNodeIndices(staticInputVarNodeIds());
  for (Int i = static_cast<Int>(duplicateIndices->size() - 1); i >= 0; --i) {
    removeStaticInputAtIndex(i);
  }

  const Int outputLb = outputVarNodeConst(0).lowerBound();
  const Int outputUb = outputVarNodeConst(0).upperBound();

  std::optional<VarNodeId> equalityVarNodeId{std::nullopt};

  for (Int i = static_cast<Int>(staticInputVarNodeIds().size()) - 1; i >= 0;
       --i) {
    const Int inputLb = staticInputVarNodeConst(i).lowerBound();
    const Int inputUb = staticInputVarNodeConst(i).upperBound();
    _lowerBound = std::max(_lowerBound, inputLb);
    if (inputLb == inputUb) {
      if (inputLb == outputUb) {
        setState(InvariantNodeState::SUBSUMED);
        return;
      }
      removeStaticInputAtIndex(i);
    } else if (inputUb < outputLb) {
      removeStaticInputAtIndex(i);
    } else if (outputLb <= inputLb && inputUb <= outputUb) {
      equalityVarNodeId = equalityVarNodeId.has_value()
                              ? NULL_NODE_ID
                              : staticInputVarNodeIds().at(i);
    }
  }
  if (equalityVarNodeId.has_value() && *equalityVarNodeId != NULL_NODE_ID) {
    while (staticInputVarNodeIds().size() > 1) {
      const size_t index =
          staticInputVarNodeIds().size() -
          (staticInputVarNodeIds().back() == *equalityVarNodeId ? 2 : 1);
      removeStaticInputAtIndex(index);
    }
  }

  if (staticInputVarNodeIds().empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayIntMaximumNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1 &&
      _lowerBound <= staticInputVarNodeConst(0).lowerBound()) {
    return true;
  }
  if (outputVarNodeConst(0).isFixed()) {
    return true;
  }
  return false;
}

bool ArrayIntMaximumNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1 &&
      _lowerBound <= staticInputVarNodeConst(0).lowerBound()) {
    invariantGraph().replaceVarNode(outputVarNodeIds().front(),
                                    staticInputVarNodeIds().front());
    return true;
  }
  assert(outputVarNodeIds().size() == 1 && outputVarNodeConst(0).isFixed());
  invariantGraph().addInvariantNode(std::make_shared<CountRelNode>(
      invariantGraph(), Int{1}, RelationType::REL_TYPE_LE,
      std::vector<VarNodeId>{staticInputVarNodeIds()},
      outputVarNodeConst(0).lowerBound(), true));
  return true;
}

void ArrayIntMaximumNode::registerOutputVars(propagation::SolverBase& solver,
                                             SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() == 1) {
    mapping.setSolverId(
        outputVarNodeIds().front(),
        solver.makeIntView<propagation::IntMaxView>(
            solver, mapping.solverId(staticInputVarNodeIds().front()),
            _lowerBound));
  } else if (!staticInputVarNodeIds().empty()) {
    if (_lowerBound > staticInputVarNodeConst(0).lowerBound()) {
      mapping.setIntermediateId(id(), solver.makeIntVar(0, 0, 0));
      mapping.setSolverId(
          outputVarNodeIds().front(),
          solver.makeIntView<propagation::IntMaxView>(
              solver, mapping.intermediateId(id()), _lowerBound));
    } else {
      makeSolverVar(outputVarNodeIds().front(), solver, mapping);
    }
  }
  assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
    return mapping.solverId(vId) != propagation::NULL_ID;
  }));
}

void ArrayIntMaximumNode::registerNode(propagation::SolverBase& solver,
                                       SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(staticInputVarNodeIds().size());
  std::ranges::transform(
      staticInputVarNodeIds(), std::back_inserter(solverVars),
      [&](const auto& node) { return mapping.solverId(node); });

  assert(mapping.solverId(outputVarNodeIds().front()) != propagation::NULL_ID);
  assert(mapping.intermediateId(id()) != propagation::NULL_ID
             ? mapping.solverId(outputVarNodeIds().front()).isView()
             : mapping.solverId(outputVarNodeIds().front()).isVar());
  solver.makeInvariant<propagation::Max>(
      solver,
      mapping.intermediateId(id()) != propagation::NULL_ID
          ? mapping.intermediateId(id())
          : mapping.solverId(outputVarNodeIds().front()),
      std::move(solverVars));
}

std::string ArrayIntMaximumNode::dotLangIdentifier() const { return "max"; }

}  // namespace atlantis::invariantgraph
