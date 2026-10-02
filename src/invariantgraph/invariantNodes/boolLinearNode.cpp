#include "atlantis/invariantgraph/invariantNodes/boolLinearNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/boolLinear.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/ifThenElseConst.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::invariantgraph {

BoolLinearNode::BoolLinearNode(InvariantGraph& graph, std::vector<Int>&& coeffs,
                               std::vector<std::shared_ptr<VarNode>>&& vars,
                               VarNode& output, const Int rhsOffset)
    : InvariantNode(graph, {output}, std::move(vars)),
      _coeffs(std::move(coeffs)),
      _rhsOffset(rhsOffset) {}

void BoolLinearNode::init() {
  InvariantNode::init();
  assert(
      invariantGraphConst().varNodeConst(outputVarNodes().front()).isIntVar());
  assert(std::ranges::none_of(
      staticInputVarNodes().begin(), staticInputVarNodes().end(),
      [&](const std::shared_ptr<VarNode>& vId) { return vId.isIntVar(); }));
}

void BoolLinearNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().bool_lin_eq(
      _coeffs, toConstraintVarIds(invariantGraphConst(), staticInputVarNodes()),
      outputVarNodeConst(0).constraintVarId(), _rhsOffset);
}

void BoolLinearNode::updateState() {
  // Remove duplicates:
  for (Int i = 0; i < static_cast<Int>(staticInputVarNodes().size()); ++i) {
    for (Int j = static_cast<Int>(staticInputVarNodes().size()) - 1; j > i;
         --j) {
      if (staticInputVarNodes().at(i) == staticInputVarNodes().at(j)) {
        _coeffs.at(i) += _coeffs.at(j);
        _coeffs.erase(_coeffs.begin() + j);
        eraseStaticInputVarNode(j);
      }
    }
  }

  // Remove fixed inputs and inputs with a coefficient of 0 as well as update
  // _offset:
  std::vector<Int> indicesToRemove;
  indicesToRemove.reserve(staticInputVarNodes().size());

  for (Int i = 0; i < static_cast<Int>(staticInputVarNodes().size()); ++i) {
    const auto& inputNode =
        invariantGraphConst().varNodeConst(staticInputVarNodes().at(i));
    if (inputNode.isFixed() || _coeffs.at(i) == 0) {
      // var i is fixed: reduce the RHS:
      _rhsOffset -= inputNode.inDomain(bool{true}) ? _coeffs.at(i) : 0;
      indicesToRemove.emplace_back(i);
    }
  }

  for (Int i = static_cast<Int>(indicesToRemove.size()) - 1; i >= 0; --i) {
    removeStaticInputVarNode(staticInputVarNodes().at(indicesToRemove.at(i)));
    _coeffs.erase(_coeffs.begin() + indicesToRemove.at(i));
  }

  if (staticInputVarNodes().empty()) {
    outputVarNode(0).fixToValue(-_rhsOffset);
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool BoolLinearNode::constrainsOutput(VarNode&) const {
  const Int lb = linearLb(invariantGraphConst(), _coeffs, staticInputVarNodes(),
                          _rhsOffset);
  const Int ub = linearUb(invariantGraphConst(), _coeffs, staticInputVarNodes(),
                          _rhsOffset);
  return !outputVarNodeConst(0).constDomain()->contains(lb, ub);
}

void BoolLinearNode::registerOutputVars(propagation::SolverBase& solver,
                                        SolverMapping& mapping) const {
  if (staticInputVarNodes().size() == 1) {
    mapping.setSolverId(
        outputVarNodes().front(),
        solver.makeIntView<propagation::IfThenElseConst>(
            solver, mapping.solverId(staticInputVarNodes().front()),
            _coeffs.front() - _rhsOffset, -_rhsOffset));
  } else if (!staticInputVarNodes().empty()) {
    if (_rhsOffset != 0) {
      makeSolverVar(outputVarNodes().front(), solver, mapping);
    } else if (mapping.intermediateId(id(), 0) == propagation::NULL_ID) {
      Int ub = 0;
      Int lb = 0;
      for (const Int c : _coeffs) {
        if (c > 0) {
          ub = overflow::saturatingAdd(ub, c);
        } else {
          lb = overflow::saturatingAdd(lb, c);
        }
      }
      mapping.setIntermediateId(id(), 0, solver.makeIntVar(lb, lb, ub));
      mapping.setSolverId(
          outputVarNodes().front(),
          solver.makeIntView<propagation::IntOffsetView>(
              solver, mapping.intermediateId(id(), 0), -_rhsOffset));
    }
  }
  assert(std::ranges::all_of(outputVarNodes().begin(), outputVarNodes().end(),
                             [&](const std::shared_ptr<VarNode>& vId) {
                               return mapping.solverId(vId) !=
                                      propagation::NULL_ID;
                             }));
}

void BoolLinearNode::registerNode(propagation::SolverBase& solver,
                                  SolverMapping& mapping) const {
  if (staticInputVarNodes().size() <= 1) {
    return;
  }
  assert(mapping.solverId(outputVarNodes().front()) != propagation::NULL_ID);
  assert(mapping.intermediateId(id(), 0) == propagation::NULL_ID
             ? mapping.solverId(outputVarNodes().front()).isVar()
             : mapping.solverId(outputVarNodes().front()).isView());
  assert(mapping.intermediateId(id()) == propagation::NULL_ID ||
         mapping.intermediateId(id()).isVar());

  std::vector<propagation::VarViewId> solverVars;
  std::ranges::transform(staticInputVarNodes(), std::back_inserter(solverVars),
                         [&](const std::shared_ptr<VarNode>& varNodeId) {
                           return mapping.solverId(varNodeId);
                         });
  solver.makeInvariant<propagation::BoolLinear>(
      solver,
      mapping.intermediateId(id()) == propagation::NULL_ID
          ? mapping.solverId(outputVarNodes().front())
          : mapping.intermediateId(id()),
      std::vector<Int>(_coeffs), std::move(solverVars));
}

const std::vector<Int>& BoolLinearNode::coeffs() const { return _coeffs; }

std::string BoolLinearNode::dotLangIdentifier() const { return "bool_linear"; }

}  // namespace atlantis::invariantgraph
