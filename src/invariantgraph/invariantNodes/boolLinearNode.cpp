#include "atlantis/invariantgraph/invariantNodes/boolLinearNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/boolLinear.hpp"
#include "atlantis/propagation/views/ifThenElseConst.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"
#include "atlantis/propagation/views/scalarView.hpp"

namespace atlantis::invariantgraph {

BoolLinearNode::BoolLinearNode(std::vector<Int>&& coeffs,
                               std::vector<VarNodeId>&& vars, VarNodeId output,
                               Int offset)
    : InvariantNode({output}, std::move(vars)),
      _coeffs(std::move(coeffs)),
      _offset(offset) {}

void BoolLinearNode::updateState(InvariantGraph& graph) {
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
    const auto& inputNode = graph.varNodeConst(staticInputVarNodeIds().at(i));
    if (inputNode.isFixed() || _coeffs.at(i) == 0) {
      _offset += inputNode.inDomain(bool{true}) ? _coeffs.at(i) : 0;
      indicesToRemove.emplace_back(i);
    }
  }

  for (Int i = indicesToRemove.size() - 1; i >= 0; --i) {
    removeStaticInputVarNode(
        graph.varNode(staticInputVarNodeIds().at(indicesToRemove.at(i))));
    _coeffs.erase(_coeffs.begin() + indicesToRemove.at(i));
  }

  // update bounds of output:
  Int lb = _offset;
  Int ub = _offset;
  for (size_t i = 0; i < staticInputVarNodeIds().size(); ++i) {
    lb += std::min<Int>(0, _coeffs.at(i));
    ub += std::max<Int>(0, _coeffs.at(i));
  }

  graph.varNode(outputVarNodeIds().front()).removeValuesBelow(lb);
  graph.varNode(outputVarNodeIds().front()).removeValuesAbove(ub);

  if (staticInputVarNodeIds().size() == 0) {
    graph.varNode(outputVarNodeIds().front()).fixToValue(_offset);
    setState(InvariantNodeState::SUBSUMED);
  }
}

void BoolLinearNode::registerOutputVars(InvariantGraph& invariantGraph,
                                        propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  if (staticInputVarNodeIds().size() == 1) {
    invariantGraph.varNode(outputVarNodeIds().front())
        .setVarId(solver.makeIntView<propagation::IfThenElseConst>(
            solver, invariantGraph.varId(staticInputVarNodeIds().front()),
            _offset + _coeffs.front(), _offset));
  } else if (_offset != 0) {
    makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
  } else if (_intermediate == propagation::NULL_ID) {
    _intermediate = solver.makeIntVar(0, 0, 0);
    invariantGraph.varNode(outputVarNodeIds().front())
        .setVarId(solver.makeIntView<propagation::IntOffsetView>(
            solver, _intermediate, _offset));
  }
}

void BoolLinearNode::registerNode(InvariantGraph& invariantGraph,
                                  propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);

  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const VarNodeId varNodeId) {
                   return invariantGraph.varId(varNodeId);
                 });
  solver.makeInvariant<propagation::BoolLinear>(
      solver,
      _intermediate == propagation::NULL_ID
          ? invariantGraph.varId(outputVarNodeIds().front())
          : _intermediate,
      std::vector<Int>(_coeffs), std::move(solverVars));
}

const std::vector<Int>& BoolLinearNode::coeffs() const { return _coeffs; }

}  // namespace atlantis::invariantgraph
