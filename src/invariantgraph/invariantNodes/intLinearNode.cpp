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
    : InvariantNode(graph, {output}, std::move(vars)),
      _coeffs(std::move(coeffs)),
      _rhsOffset(rhsOffset) {}

void IntLinearNode::init() {
  InvariantNode::init();
  assert(
      invariantGraphConst().varNodeConst(outputVarNodes().front()).isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodes().begin(), staticInputVarNodes().end(),
      [&](const std::shared_ptr<VarNode>& vId) { return vId.isIntVar(); }));
}

void IntLinearNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().int_lin_eq(
      _coeffs, toConstraintVarIds(invariantGraphConst(), staticInputVarNodes()),
      outputVarNodeConst(0).constraintVarId(), _rhsOffset);
}

void IntLinearNode::updateState() {
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

  std::vector<Int> indicesToRemove;
  indicesToRemove.reserve(staticInputVarNodes().size());

  for (Int i = 0; i < static_cast<Int>(staticInputVarNodes().size()); ++i) {
    const auto& inputNode =
        invariantGraphConst().varNodeConst(staticInputVarNodes().at(i));
    if (inputNode.isFixed() || _coeffs.at(i) == 0) {
      // var i is fixed: reduce the RHS:
      _rhsOffset -= _coeffs.at(i) * inputNode.lowerBound();
      indicesToRemove.emplace_back(i);
    }
  }

  for (Int i = static_cast<Int>(indicesToRemove.size()) - 1; i >= 0; --i) {
    removeStaticInputVarNode(staticInputVarNodes().at(indicesToRemove.at(i)));
    _coeffs.erase(_coeffs.begin() + indicesToRemove.at(i));
  }

  if (staticInputVarNodes().empty()) {
    assert(outputVarNode(0).isFixed());
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntLinearNode::constrainsOutput(VarNode&) const {
  const Int lb = linearLb(invariantGraphConst(), _coeffs, staticInputVarNodes(),
                          _rhsOffset);
  const Int ub = linearUb(invariantGraphConst(), _coeffs, staticInputVarNodes(),
                          _rhsOffset);
  return !outputVarNodeConst(0).constDomain()->contains(lb, ub);
}

std::pair<size_t, size_t> IntLinearNode::implicitRank() const {
  return {rank::IMPLICIT_RANK_BOOL_LIN_EQ,
          staticInputVarNodes().size() + outputVarNodes().size()};
}

bool IntLinearNode::canBeMadeImplicit() const {
  return std::ranges::all_of(
             _coeffs.begin(), _coeffs.end(),
             [](const Int& coeff) { return std::abs(coeff) == 1; }) &&
         std::ranges::all_of(staticInputVarNodes().begin(),
                             staticInputVarNodes().end(),
                             [&](const std::shared_ptr<VarNode>& vId) {
                               return invariantGraphConst()
                                   .varNodeConst(vId)
                                   .definingNodes()
                                   .empty();
                             }) &&
         invariantGraphConst()
             .varNodeConst(outputVarNodes().front())
             .definingNodes()
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
        outputVarNodes().front(),
        solver.makeIntView<propagation::ScalarView>(
            solver, mapping.solverId(staticInputVarNodes().front()),
            _coeffs.front(), -_rhsOffset));
    return;
  }
  if (!staticInputVarNodes().empty()) {
    if (_rhsOffset != 0) {
      if (mapping.intermediateId(id()) == propagation::NULL_ID) {
        const auto& outputNode =
            invariantGraphConst().varNodeConst(outputVarNodes().front());
        const Int intermediateLb =
            overflow::saturatingSub(outputNode.lowerBound(), _rhsOffset);
        const Int intermediateUb =
            overflow::saturatingSub(outputNode.upperBound(), _rhsOffset);
        mapping.setIntermediateId(
            id(), solver.makeIntVar(std::max(intermediateLb,
                                             std::min(intermediateUb, Int{0})),
                                    intermediateLb, intermediateUb));
      }
      mapping.setSolverId(
          outputVarNodes().front(),
          solver.makeIntView<propagation::IntOffsetView>(
              solver, mapping.intermediateId(id()), -_rhsOffset));
    } else {
      makeSolverVar(outputVarNodes().front(), solver, mapping);
      assert(mapping.solverId(outputVarNodes().front()).isVar());
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
  assert(mapping.solverId(outputVarNodes().front()) != propagation::NULL_ID);
  assert(mapping.intermediateId(id()) == propagation::NULL_ID
             ? mapping.solverId(outputVarNodes().front()).isVar()
             : mapping.solverId(outputVarNodes().front()).isView());
  assert(mapping.intermediateId(id()) == propagation::NULL_ID ||
         mapping.intermediateId(id()).isVar());

  std::vector<propagation::VarViewId> solverVars;
  std::ranges::transform(
      staticInputVarNodes(), std::back_inserter(solverVars),
      [&](const std::shared_ptr<VarNode>& varNodeId) {
        assert(mapping.solverId(varNodeId) != propagation::NULL_ID);
        return mapping.solverId(varNodeId);
      });
  solver.makeInvariant<propagation::Linear>(
      solver,
      mapping.intermediateId(id()) == propagation::NULL_ID
          ? mapping.solverId(outputVarNodes().front())
          : mapping.intermediateId(id()),
      std::vector<Int>(_coeffs), std::move(solverVars));
}

const std::vector<Int>& IntLinearNode::coeffs() const { return _coeffs; }

std::string IntLinearNode::dotLangIdentifier() const { return "int_linear"; }

}  // namespace atlantis::invariantgraph
