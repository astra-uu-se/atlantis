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
    : InvariantNode(graph, {output.ptr()}, std::move(vars)),
      _coeffs(std::move(coeffs)),
      _rhsOffset(rhsOffset) {}

void BoolLinearNode::init() {
  InvariantNode::init();
  assert(
      outputVarNode(0).isIntVar());
  assert(std::ranges::none_of(
      staticInputVarNodes(),
      [&](const std::shared_ptr<VarNode>& vNode) { return vNode->isIntVar(); }));
}

void BoolLinearNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().bool_lin_eq(
      _coeffs, toConstraintVarIds(invariantGraphConst(), staticInputVarNodes()),
      outputVarNode(0).constraintVarId(), _rhsOffset);
}

void BoolLinearNode::updateState() {
  // Remove duplicates:
  for (Int i = 0; i < static_cast<Int>(staticInputVarNodes().size()); ++i) {
    for (Int j = static_cast<Int>(staticInputVarNodes().size()) - 1; j > i;
         --j) {
      if (&staticInputVarNode(i) == &staticInputVarNode(j)) {
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
    if (staticInputVarNode(i).isFixed() || _coeffs.at(i) == 0) {
      // var i is fixed: reduce the RHS:
      _rhsOffset -= staticInputVarNode(i).inDomain(bool{true}) ? _coeffs.at(i) : 0;
      indicesToRemove.emplace_back(i);
    }
  }

  for (Int i = static_cast<Int>(indicesToRemove.size()) - 1; i >= 0; --i) {
    removeStaticInputVarNode(staticInputVarNode(indicesToRemove.at(i)));
    _coeffs.erase(_coeffs.begin() + indicesToRemove.at(i));
  }

  if (staticInputVarNodes().empty()) {
    outputVarNode(0).fixToValue(-_rhsOffset);
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool BoolLinearNode::constrainsOutput(const VarNode&) const {
  const Int lb = linearLb(invariantGraphConst(), _coeffs, staticInputVarNodes(),
                          _rhsOffset);
  const Int ub = linearUb(invariantGraphConst(), _coeffs, staticInputVarNodes(),
                          _rhsOffset);
  return !outputVarNode(0).constDomain()->contains(lb, ub);
}

void BoolLinearNode::registerOutputVars(propagation::SolverBase& solver,
                                        SolverMapping& mapping) const {
  if (staticInputVarNodes().size() == 1) {
    mapping.setSolverId(
        outputVarNode(0),
        solver.makeIntView<propagation::IfThenElseConst>(
            solver, mapping.solverId(staticInputVarNodes().front()),
            _coeffs.front() - _rhsOffset, -_rhsOffset));
  } else if (!staticInputVarNodes().empty()) {
    if (_rhsOffset != 0) {
      makeSolverVar(outputVarNode(0), solver, mapping);
    } else if (mapping.intermediateId(ptrConst(), 0) == propagation::NULL_ID) {
      Int ub = 0;
      Int lb = 0;
      for (const Int c : _coeffs) {
        if (c > 0) {
          ub = overflow::saturatingAdd(ub, c);
        } else {
          lb = overflow::saturatingAdd(lb, c);
        }
      }
      mapping.setIntermediateId(ptrConst(), 0, solver.makeIntVar(lb, lb, ub));
      mapping.setSolverId(
          outputVarNode(0),
          solver.makeIntView<propagation::IntOffsetView>(
              solver, mapping.intermediateId(ptrConst(), 0), -_rhsOffset));
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
  assert(mapping.solverId(outputVarNode(0)) != propagation::NULL_ID);
  assert(mapping.intermediateId(ptrConst(), 0) == propagation::NULL_ID
             ? mapping.solverId(outputVarNode(0)).isVar()
             : mapping.solverId(outputVarNode(0)).isView());
  assert(mapping.intermediateId(ptrConst()) == propagation::NULL_ID ||
         mapping.intermediateId(ptrConst()).isVar());

  std::vector<propagation::VarViewId> solverVars;
  std::ranges::transform(staticInputVarNodes(), std::back_inserter(solverVars),
                         [&](const std::shared_ptr<VarNode>& varNodeId) {
                           return mapping.solverId(varNodeId);
                         });
  solver.makeInvariant<propagation::BoolLinear>(
      solver,
      mapping.intermediateId(ptrConst()) == propagation::NULL_ID
          ? mapping.solverId(outputVarNode(0))
          : mapping.intermediateId(ptrConst()),
      std::vector<Int>(_coeffs), std::move(solverVars));
}

const std::vector<Int>& BoolLinearNode::coeffs() const { return _coeffs; }

std::string BoolLinearNode::dotLangIdentifier() const { return "bool_linear"; }

}  // namespace atlantis::invariantgraph
