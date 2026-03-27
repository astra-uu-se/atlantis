#include "atlantis/invariantgraph/invariantNodes/boolLinearNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/boolLinear.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/ifThenElseConst.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"

namespace atlantis::invariantgraph {

BoolLinearNode::BoolLinearNode(InvariantGraph& graph, std::vector<Int>&& coeffs,
                               std::vector<VarNodeId>&& vars, VarNodeId output,
                               Int offset)
    : InvariantNode(graph, {output}, std::move(vars)),
      _coeffs(std::move(coeffs)),
      _offset(offset) {}

void BoolLinearNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(invariantGraphConst()
             .varNodeConst(outputVarNodeIds().front())
             .isIntVar());
  assert(std::ranges::none_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void BoolLinearNode::updateState() {
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

  // Remove fixed inputs and inputs with a coefficient of 0 as well as update
  // _offset:
  std::vector<Int> indicesToRemove;
  indicesToRemove.reserve(staticInputVarNodeIds().size());

  for (Int i = 0; i < static_cast<Int>(staticInputVarNodeIds().size()); ++i) {
    const auto& inputNode =
        invariantGraphConst().varNodeConst(staticInputVarNodeIds().at(i));
    if (inputNode.isFixed() || _coeffs.at(i) == 0) {
      _offset += inputNode.inDomain(bool{true}) ? _coeffs.at(i) : 0;
      indicesToRemove.emplace_back(i);
    }
  }

  for (Int i = static_cast<Int>(indicesToRemove.size()) - 1; i >= 0; --i) {
    removeStaticInputVarNode(staticInputVarNodeIds().at(indicesToRemove.at(i)));
    _coeffs.erase(_coeffs.begin() + indicesToRemove.at(i));
  }

  if (staticInputVarNodeIds().empty()) {
    invariantGraph().varNode(outputVarNodeIds().front()).fixToValue(_offset);
    setState(InvariantNodeState::SUBSUMED);
  }
}

void BoolLinearNode::registerOutputVars(propagation::SolverBase& solver,
                                        SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() == 1) {
    mapping.setSolverId(
        outputVarNodeIds().front(),
        solver.makeIntView<propagation::IfThenElseConst>(
            solver, mapping.solverId(staticInputVarNodeIds().front()),
            _offset + _coeffs.front(), _offset));
  } else if (!staticInputVarNodeIds().empty()) {
    if (_offset != 0) {
      makeSolverVar(outputVarNodeIds().front(), solver, mapping);
    } else if (mapping.intermediateId(id(), 0) == propagation::NULL_ID) {
      mapping.setIntermediateId(id(), 0, solver.makeIntVar(0, 0, 0));
      mapping.setSolverId(
          outputVarNodeIds().front(),
          solver.makeIntView<propagation::IntOffsetView>(
              solver, mapping.intermediateId(id(), 0), _offset));
    }
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void BoolLinearNode::registerNode(propagation::SolverBase& solver,
                                  SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  assert(mapping.solverId(outputVarNodeIds().front()) != propagation::NULL_ID);
  assert(mapping.intermediateId(id(), 0) == propagation::NULL_ID
             ? mapping.solverId(outputVarNodeIds().front()).isVar()
             : mapping.solverId(outputVarNodeIds().front()).isView());
  assert(mapping.intermediateId(id()) == propagation::NULL_ID ||
         mapping.intermediateId(id()).isVar());

  std::vector<propagation::VarViewId> solverVars;
  std::ranges::transform(
      staticInputVarNodeIds(), std::back_inserter(solverVars),
      [&](const VarNodeId varNodeId) { return mapping.solverId(varNodeId); });
  solver.makeInvariant<propagation::BoolLinear>(
      solver,
      mapping.intermediateId(id()) == propagation::NULL_ID
          ? mapping.solverId(outputVarNodeIds().front())
          : mapping.intermediateId(id()),
      std::vector<Int>(_coeffs), std::move(solverVars));
}

const std::vector<Int>& BoolLinearNode::coeffs() const { return _coeffs; }

std::string BoolLinearNode::dotLangIdentifier() const { return "bool_linear"; }

}  // namespace atlantis::invariantgraph
