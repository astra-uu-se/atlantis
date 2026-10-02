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

ArrayIntMinimumNode::ArrayIntMinimumNode(InvariantGraph& graph, VarNode& a,
                                         VarNode& b, VarNode& output)
    : ArrayIntMinimumNode(graph, std::vector<std::shared_ptr<VarNode>>{a, b},
                          output) {}

ArrayIntMinimumNode::ArrayIntMinimumNode(
    InvariantGraph& graph, std::vector<std::shared_ptr<VarNode>>&& vars,
    VarNode& output)
    : InvariantNode(graph, {output}, std::move(vars)),
      _upperBound(std::numeric_limits<Int>::max()) {}

void ArrayIntMinimumNode::init() {
  InvariantNode::init();
  assert(
      invariantGraphConst().varNodeConst(outputVarNodes().front()).isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodes().begin(), staticInputVarNodes().end(),
      [&](const std::shared_ptr<VarNode>& vId) { return vId.isIntVar(); }));
}

void ArrayIntMinimumNode::postConstraint() {
  constraintSolver().array_int_minimum(
      toConstraintVarIds(invariantGraphConst(), staticInputVarNodes()),
      outputVarNode(0).constraintVarId());
}

void ArrayIntMinimumNode::updateState() {
  InvariantNode::updateState();
  const auto duplicateIndices = duplicateVarNodeIndices(staticInputVarNodes());
  for (Int i = static_cast<Int>(duplicateIndices->size() - 1); i >= 0; --i) {
    removeStaticInputAtIndex(i);
  }

  const Int outputLb = outputVarNodeConst(0).lowerBound();
  const Int outputUb = outputVarNodeConst(0).upperBound();

  for (Int i = static_cast<Int>(staticInputVarNodes().size()) - 1; i >= 0;
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
    }
  }
  for (Int i = static_cast<Int>(staticInputVarNodes().size()) - 1; i >= 0;
       --i) {
    if (staticInputVarNodeConst(i).lowerBound() >= _upperBound) {
      removeStaticInputAtIndex(i);
    }
  }
  assert(_upperBound >= outputUb);
  if (staticInputVarNodes().empty()) {
    outputVarNode(0).tightenDomainType();
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayIntMinimumNode::constrainsOutput(VarNode&) const {
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::max();
  for (const auto vId : staticInputVarNodes()) {
    lb = std::min(lb, varNodeConst(vId).lowerBound());
    ub = std::min(ub, varNodeConst(vId).upperBound());
  }
  if (ub > _upperBound) {
    return true;
  }
  return !outputVarNodeConst(0).constDomain()->contains(lb, ub);
}

bool ArrayIntMinimumNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }
  if (staticInputVarNodes().size() == 1 &&
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
  if (staticInputVarNodes().size() == 1 &&
      _upperBound >= staticInputVarNodeConst(0).upperBound()) {
    invariantGraph().replaceVarNode(outputVarNodes().front(),
                                    staticInputVarNodes().front());
    return true;
  }
  assert(outputVarNodes().size() == 1 && outputVarNodeConst(0).isFixed());
  invariantGraph().addInvariantNode(std::make_shared<CountRelNode>(
      invariantGraph(), Int{1}, RelationType::REL_TYPE_LE,
      std::vector<std::shared_ptr<VarNode>>{staticInputVarNodes()},
      outputVarNodeConst(0).upperBound(), true));
  return true;
}

void ArrayIntMinimumNode::registerOutputVars(propagation::SolverBase& solver,
                                             SolverMapping& mapping) const {
  if (staticInputVarNodes().size() == 1) {
    mapping.setSolverId(
        outputVarNodes().front(),
        solver.makeIntView<propagation::IntMinView>(
            solver, mapping.solverId(staticInputVarNodes().front()),
            _upperBound));
  } else if (!staticInputVarNodes().empty()) {
    if (_upperBound > staticInputVarNodeConst(0).lowerBound()) {
      mapping.setIntermediateId(id(), solver.makeIntVar(0, 0, 0));
      mapping.setSolverId(
          outputVarNodes().front(),
          solver.makeIntView<propagation::IntMinView>(
              solver, mapping.intermediateId(id()), _upperBound));
    } else {
      makeSolverVar(outputVarNodes().front(), solver, mapping);
    }
  }
  assert(std::ranges::all_of(
      outputVarNodes(), [&](const std::shared_ptr<VarNode>& vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void ArrayIntMinimumNode::registerNode(propagation::SolverBase& solver,
                                       SolverMapping& mapping) const {
  if (staticInputVarNodes().size() <= 1) {
    return;
  }
  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(staticInputVarNodes().size());
  std::ranges::transform(
      staticInputVarNodes().begin(), staticInputVarNodes().end(),
      std::back_inserter(solverVars),
      [&](const auto& node) { return mapping.solverId(node); });

  assert(mapping.solverId(outputVarNodes().front()) != propagation::NULL_ID);
  assert(mapping.intermediateId(id()) != propagation::NULL_ID
             ? mapping.solverId(outputVarNodes().front()).isView()
             : mapping.solverId(outputVarNodes().front()).isVar());
  solver.makeInvariant<propagation::Min>(
      solver,
      mapping.intermediateId(id()) != propagation::NULL_ID
          ? mapping.intermediateId(id())
          : mapping.solverId(outputVarNodes().front()),
      std::move(solverVars));
}

std::string ArrayIntMinimumNode::dotLangIdentifier() const { return "min"; }

}  // namespace atlantis::invariantgraph
