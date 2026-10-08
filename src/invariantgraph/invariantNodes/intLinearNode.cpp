#include "atlantis/invariantgraph/invariantNodes/intLinearNode.hpp"

#include <algorithm>
#include <utility>

#include "../implicitRanks.hpp"
#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/intLinEqImplicitNode.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/views/intScalarNode.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"
#include "atlantis/propagation/views/scalarView.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::invariantgraph {

IntLinearNode::IntLinearNode(InvariantGraph& graph, std::vector<Int>&& coeffs,
                             std::vector<std::shared_ptr<VarNode>>&& vars,
                             VarNode& output, const Int rhsOffset)
    : InvariantNode(graph, {output.ptr()}, std::move(vars)),
      _coeffs(std::move(coeffs)),
      _rhsOffset(rhsOffset) {}

void IntLinearNode::init() {
  InvariantNode::init();
  assert(outputVarNode(0).isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodes().begin(), staticInputVarNodes().end(),
      [&](const std::shared_ptr<VarNode>& vNode) { return vNode->isIntVar(); }));
}

void IntLinearNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().int_lin_eq(
      _coeffs, toConstraintVarIds(invariantGraphConst(), staticInputVarNodes()),
      outputVarNode(0).constraintVarId(), _rhsOffset);
}

void IntLinearNode::updateState() {
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

  std::vector<Int> indicesToRemove;
  indicesToRemove.reserve(staticInputVarNodes().size());

  for (Int i = 0; i < static_cast<Int>(staticInputVarNodes().size()); ++i) {
    const auto& inputNode = staticInputVarNode(i);
    if (inputNode.isFixed() || _coeffs.at(i) == 0) {
      // var i is fixed: reduce the RHS:
      _rhsOffset -= _coeffs.at(i) * inputNode.lowerBound();
      indicesToRemove.emplace_back(i);
    }
  }

  for (Int i = static_cast<Int>(indicesToRemove.size()) - 1; i >= 0; --i) {
    removeStaticInputVarNode(staticInputVarNode(indicesToRemove.at(i)));
    _coeffs.erase(_coeffs.begin() + indicesToRemove.at(i));
  }

  if (staticInputVarNodes().empty()) {
    assert(outputVarNode(0).isFixed());
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntLinearNode::constrainsOutput(const VarNode&) const {
  const Int lb = linearLb(invariantGraphConst(), _coeffs, staticInputVarNodes(),
                          _rhsOffset);
  const Int ub = linearUb(invariantGraphConst(), _coeffs, staticInputVarNodes(),
                          _rhsOffset);
  return !outputVarNode(0).constDomain()->contains(lb, ub);
}

std::pair<size_t, size_t> IntLinearNode::implicitRank() const {
  return {rank::IMPLICIT_RANK_BOOL_LIN_EQ,
          staticInputVarNodes().size() + outputVarNodes().size()};
}

bool IntLinearNode::canBeMadeImplicit() const {
  return std::ranges::all_of(
             _coeffs.begin(), _coeffs.end(),
             [](const Int& coeff) { return std::abs(coeff) == 1; }) &&
         std::ranges::all_of(staticInputVarNodes(),
                             [&](const std::shared_ptr<VarNode>& varNode) {
                               return varNode->definingNodes()
                                   .empty();
                             }) &&
         outputVarNode(0).definingNodes()
             .empty();
}

bool IntLinearNode::makeImplicit() {
  if (!canBeMadeImplicit()) {
    return false;
  }

  _coeffs.emplace_back(-1);

  std::vector<std::shared_ptr<VarNode>> inputVarNodeIds(staticInputVarNodes());
  inputVarNodeIds.emplace_back(outputVarNodes().front());

  invariantGraph().addImplicitConstraintNode(
      std::make_shared<IntLinEqImplicitNode>(
          invariantGraph(), std::move(_coeffs), std::move(inputVarNodeIds),
          _rhsOffset));

  return true;
}

void IntLinearNode::registerOutputVars(propagation::SolverBase& solver,
                                       SolverMapping& mapping) const {
  if (staticInputVarNodes().size() == 1) {
    // The rhs offset needs to be reduced from the lhs sum to equal the output:
    mapping.setSolverId(
        outputVarNode(0),
        solver.makeIntView<propagation::ScalarView>(
            solver, mapping.solverId(staticInputVarNodes().front()),
            _coeffs.front(), -_rhsOffset));
    return;
  }
  if (!staticInputVarNodes().empty()) {
    if (_rhsOffset != 0) {
      if (mapping.intermediateId(ptrConst()) == propagation::NULL_ID) {
        const Int intermediateLb =
            overflow::saturatingSub(outputVarNode(0).lowerBound(), _rhsOffset);
        const Int intermediateUb =
            overflow::saturatingSub(outputVarNode(0).upperBound(), _rhsOffset);
        mapping.setIntermediateId(
            ptrConst(), solver.makeIntVar(std::max(intermediateLb,
                                             std::min(intermediateUb, Int{0})),
                                    intermediateLb, intermediateUb));
      }
      mapping.setSolverId(
          outputVarNode(0),
          solver.makeIntView<propagation::IntOffsetView>(
              solver, mapping.intermediateId(ptrConst()), -_rhsOffset));
    } else {
      makeSolverVar(outputVarNode(0), solver, mapping);
      assert(mapping.solverId(outputVarNode(0)).isVar());
    }
  }
  assert(std::ranges::all_of(outputVarNodes().begin(), outputVarNodes().end(),
                             [&](const std::shared_ptr<VarNode>& vId) {
                               return mapping.solverId(vId) !=
                                      propagation::NULL_ID;
                             }));
}

void IntLinearNode::registerNode(propagation::SolverBase& solver,
                                 SolverMapping& mapping) const {
  if (staticInputVarNodes().size() <= 1) {
    return;
  }
  assert(mapping.solverId(outputVarNode(0)) != propagation::NULL_ID);
  assert(mapping.intermediateId(ptrConst()) == propagation::NULL_ID
             ? mapping.solverId(outputVarNode(0)).isVar()
             : mapping.solverId(outputVarNode(0)).isView());
  assert(mapping.intermediateId(ptrConst()) == propagation::NULL_ID ||
         mapping.intermediateId(ptrConst()).isVar());

  std::vector<propagation::VarViewId> solverVars;
  std::ranges::transform(
      staticInputVarNodes(), std::back_inserter(solverVars),
      [&](const std::shared_ptr<VarNode>& varNodeId) {
        assert(mapping.solverId(varNodeId) != propagation::NULL_ID);
        return mapping.solverId(varNodeId);
      });
  solver.makeInvariant<propagation::Linear>(
      solver,
      mapping.intermediateId(ptrConst()) == propagation::NULL_ID
          ? mapping.solverId(outputVarNode(0))
          : mapping.intermediateId(ptrConst()),
      std::vector<Int>(_coeffs), std::move(solverVars));
}

const std::vector<Int>& IntLinearNode::coeffs() const { return _coeffs; }

std::string IntLinearNode::dotLangIdentifier() const { return "int_linear"; }

}  // namespace atlantis::invariantgraph
