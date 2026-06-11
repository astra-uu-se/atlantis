#include "atlantis/invariantgraph/invariantNodes/arrayIntMinimumNode.hpp"

#include <algorithm>
#include <limits>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/countRelNode.hpp"
#include "atlantis/propagation/invariants/min.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/intMinView.hpp"

namespace atlantis::invariantgraph {

ArrayIntMinimumNode::ArrayIntMinimumNode(InvariantGraph& graph,
                                         const VarNodeId a, const VarNodeId b,
                                         const VarNodeId output)
    : ArrayIntMinimumNode(graph, std::vector<VarNodeId>{a, b}, output) {}

ArrayIntMinimumNode::ArrayIntMinimumNode(InvariantGraph& graph,
                                         std::vector<VarNodeId>&& vars,
                                         const VarNodeId output)
    : InvariantNode(graph, {output}, std::move(vars)),
      _upperBound(std::numeric_limits<Int>::max()) {}

void ArrayIntMinimumNode::init(const InvariantNodeId id) {
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

void ArrayIntMinimumNode::postConstraint() {
  constraintSolver().array_int_minimum(
      toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
      outputVarNode(0).constraintVarId());
}

void ArrayIntMinimumNode::updateState() {
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
    _upperBound = std::min(_upperBound, inputUb);
    if (inputLb == inputUb) {
      if (inputLb == outputLb) {
        setState(InvariantNodeState::SUBSUMED);
        return;
      }
      removeStaticInputAtIndex(i);
    } else if (inputLb > outputUb) {
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

bool ArrayIntMinimumNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1 &&
      _upperBound >= staticInputVarNodeConst(0).upperBound()) {
    return true;
  }
  if (outputVarNodeConst(0).isFixed()) {
    return true;
  }
  return false;
}

bool ArrayIntMinimumNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1 &&
      _upperBound >= staticInputVarNodeConst(0).upperBound()) {
    invariantGraph().replaceVarNode(outputVarNodeIds().front(),
                                    staticInputVarNodeIds().front());
    return true;
  }
  assert(outputVarNodeIds().size() == 1 && outputVarNodeConst(0).isFixed());
  invariantGraph().addInvariantNode(std::make_shared<CountRelNode>(
      invariantGraph(), Int{1}, RelationType::REL_TYPE_LE,
      std::vector<VarNodeId>{staticInputVarNodeIds()},
      outputVarNodeConst(0).upperBound(), true));
  return true;
}

void ArrayIntMinimumNode::registerOutputVars(propagation::SolverBase& solver,
                                             SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() == 1) {
    mapping.setSolverId(
        outputVarNodeIds().front(),
        solver.makeIntView<propagation::IntMinView>(
            solver, mapping.solverId(staticInputVarNodeIds().front()),
            _upperBound));
  } else if (!staticInputVarNodeIds().empty()) {
    if (_upperBound > staticInputVarNodeConst(0).lowerBound()) {
      mapping.setIntermediateId(id(), solver.makeIntVar(0, 0, 0));
      mapping.setSolverId(
          outputVarNodeIds().front(),
          solver.makeIntView<propagation::IntMinView>(
              solver, mapping.intermediateId(id()), _upperBound));
    } else {
      makeSolverVar(outputVarNodeIds().front(), solver, mapping);
    }
  }
  assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
    return mapping.solverId(vId) != propagation::NULL_ID;
  }));
}

void ArrayIntMinimumNode::registerNode(propagation::SolverBase& solver,
                                       SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(staticInputVarNodeIds().size());
  std::ranges::transform(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      std::back_inserter(solverVars),
      [&](const auto& node) { return mapping.solverId(node); });

  assert(mapping.solverId(outputVarNodeIds().front()) != propagation::NULL_ID);
  assert(mapping.intermediateId(id()) != propagation::NULL_ID
             ? mapping.solverId(outputVarNodeIds().front()).isView()
             : mapping.solverId(outputVarNodeIds().front()).isVar());
  solver.makeInvariant<propagation::Min>(
      solver,
      mapping.intermediateId(id()) != propagation::NULL_ID
          ? mapping.intermediateId(id())
          : mapping.solverId(outputVarNodeIds().front()),
      std::move(solverVars));
}

std::string ArrayIntMinimumNode::dotLangIdentifier() const { return "min"; }

}  // namespace atlantis::invariantgraph
