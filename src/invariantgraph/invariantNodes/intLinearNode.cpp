#include "atlantis/invariantgraph/invariantNodes/intLinearNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
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
                             std::vector<VarNodeId>&& vars, VarNodeId output,
                             Int offset)
    : InvariantNode(graph, {output}, std::move(vars)),
      _coeffs(std::move(coeffs)),
      _offset(offset) {}

void IntLinearNode::init(InvariantNodeId id) {
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

void IntLinearNode::updateState() {
  // Remove duplicates:
  for (Int i = 0; i < static_cast<Int>(staticInputVarNodeIds().size()); ++i) {
    for (Int j = static_cast<Int>(staticInputVarNodeIds().size()) - 1; j > i;
         --j) {
      if (staticInputVarNodeIds().at(i) == staticInputVarNodeIds().at(j)) {
        _coeffs.at(i) += _coeffs.at(j);
        _coeffs.erase(_coeffs.begin() + j);
        eraseStaticInputVarNode(j);
      }
    }
  }

  std::vector<Int> indicesToRemove;
  indicesToRemove.reserve(staticInputVarNodeIds().size());

  for (Int i = 0; i < static_cast<Int>(staticInputVarNodeIds().size()); ++i) {
    const auto& inputNode =
        invariantGraphConst().varNodeConst(staticInputVarNodeIds().at(i));
    if (inputNode.isFixed() || _coeffs.at(i) == 0) {
      _offset += _coeffs.at(i) * inputNode.lowerBound();
      indicesToRemove.emplace_back(i);
    }
  }

  for (Int i = static_cast<Int>(indicesToRemove.size()) - 1; i >= 0; --i) {
    removeStaticInputVarNode(staticInputVarNodeIds().at(indicesToRemove.at(i)));
    _coeffs.erase(_coeffs.begin() + indicesToRemove.at(i));
  }

  auto& outputNode = invariantGraph().varNode(outputVarNodeIds().front());

  if (staticInputVarNodeIds().empty()) {
    outputNode.fixToValue(_offset);
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntLinearNode::canBeMadeImplicit() const {
  return std::ranges::all_of(
             _coeffs.begin(), _coeffs.end(),
             [](const Int& coeff) { return std::abs(coeff) == 1; }) &&
         std::ranges::all_of(staticInputVarNodeIds().begin(),
                             staticInputVarNodeIds().end(),
                             [&](const VarNodeId vId) {
                               return invariantGraphConst()
                                   .varNodeConst(vId)
                                   .definingNodes()
                                   .empty();
                             }) &&
         invariantGraphConst()
             .varNodeConst(outputVarNodeIds().front())
             .definingNodes()
             .empty();
}

bool IntLinearNode::makeImplicit() {
  if (!canBeMadeImplicit()) {
    return false;
  }

  _coeffs.emplace_back(-1);

  std::vector<VarNodeId> inputVarNodeIds(staticInputVarNodeIds());
  inputVarNodeIds.emplace_back(outputVarNodeIds().front());

  invariantGraph().addImplicitConstraintNode(
      std::make_shared<IntLinEqImplicitNode>(
          invariantGraph(), std::move(_coeffs), std::move(inputVarNodeIds),
          _offset));

  return true;
}

void IntLinearNode::registerOutputVars(propagation::SolverBase& solver,
                                       SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() == 1) {
    mapping.setSolverId(
        outputVarNodeIds().front(),
        solver.makeIntView<propagation::ScalarView>(
            solver, mapping.solverId(staticInputVarNodeIds().front()),
            _coeffs.front(), _offset));
    return;
  }
  if (!staticInputVarNodeIds().empty()) {
    if (_offset != 0) {
      if (mapping.intermediateId(id()) == propagation::NULL_ID) {
        const auto& outputNode =
            invariantGraphConst().varNodeConst(outputVarNodeIds().front());
        const Int intermediateLb =
            overflow::saturatingSub(outputNode.lowerBound(), _offset);
        const Int intermediateUb =
            overflow::saturatingSub(outputNode.upperBound(), _offset);
        mapping.setIntermediateId(
            id(), solver.makeIntVar(std::max(intermediateLb,
                                             std::min(intermediateUb, Int{0})),
                                    intermediateLb, intermediateUb));
      }
      mapping.setSolverId(outputVarNodeIds().front(),
                          solver.makeIntView<propagation::IntOffsetView>(
                              solver, mapping.intermediateId(id()), _offset));
    } else {
      makeSolverVar(outputVarNodeIds().front(), solver, mapping);
      assert(mapping.solverId(outputVarNodeIds().front()).isVar());
    }
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void IntLinearNode::registerNode(propagation::SolverBase& solver,
                                 SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  assert(mapping.solverId(outputVarNodeIds().front()) != propagation::NULL_ID);
  assert(mapping.intermediateId(id()) == propagation::NULL_ID
             ? mapping.solverId(outputVarNodeIds().front()).isVar()
             : mapping.solverId(outputVarNodeIds().front()).isView());
  assert(mapping.intermediateId(id()) == propagation::NULL_ID ||
         mapping.intermediateId(id()).isVar());

  std::vector<propagation::VarViewId> solverVars;
  std::ranges::transform(
      staticInputVarNodeIds(), std::back_inserter(solverVars),
      [&](const VarNodeId varNodeId) {
        assert(mapping.solverId(varNodeId) != propagation::NULL_ID);
        return mapping.solverId(varNodeId);
      });
  solver.makeInvariant<propagation::Linear>(
      solver,
      mapping.intermediateId(id()) == propagation::NULL_ID
          ? mapping.solverId(outputVarNodeIds().front())
          : mapping.intermediateId(id()),
      std::vector<Int>(_coeffs), std::move(solverVars));
}

const std::vector<Int>& IntLinearNode::coeffs() const { return _coeffs; }

std::string IntLinearNode::dotLangIdentifier() const { return "int_linear"; }

}  // namespace atlantis::invariantgraph
