#include "atlantis/invariantgraph/invariantNodes/boolLinearNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/boolLinear.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"
#include "atlantis/propagation/views/scalarView.hpp"

namespace atlantis::invariantgraph {

static std::pair<std::vector<Int>, std::vector<VarNodeId>> fixInputs(
    std::vector<Int>&& coeffs, std::vector<VarNodeId>&& vars) {
  if (coeffs.size() != vars.size()) {
    throw std::runtime_error("Number of coefficients and variables must match");
  }
  for (size_t i = 0; i < coeffs.size() - 1; ++i) {
    if (coeffs.at(i) == 0) {
      continue;
    }
    for (size_t j = i + 1; j < coeffs.size(); ++j) {
      if (vars.at(i) == vars.at(j)) {
        coeffs.at(i) += coeffs.at(j);
        coeffs.at(j) = 0;
      }
    }
  }
  for (Int i = static_cast<Int>(coeffs.size()) - 1; i >= 0; --i) {
    if (coeffs.at(i) == 0) {
      coeffs.erase(coeffs.begin() + i);
      vars.erase(vars.begin() + i);
    }
  }
  return {std::move(coeffs), std::move(vars)};
}

BoolLinearNode::BoolLinearNode(
    std::pair<std::vector<Int>, std::vector<VarNodeId>>&& coeffsAndVars,
    VarNodeId output, Int offset)
    : InvariantNode({output}, std::move(coeffsAndVars.second)),
      _coeffs(std::move(coeffsAndVars.first)),
      _offset(offset) {}

BoolLinearNode::BoolLinearNode(std::vector<Int>&& coeffs,
                               std::vector<VarNodeId>&& vars, VarNodeId output,
                               Int offset)
    : BoolLinearNode(fixInputs(std::move(coeffs), std::move(vars)), output,
                     offset) {}

void BoolLinearNode::updateState(InvariantGraph& graph) {
  std::vector<Int> indicesToRemove;
  indicesToRemove.reserve(staticInputVarNodeIds().size());

  for (Int i = 0; i < static_cast<Int>(staticInputVarNodeIds().size()); ++i) {
    const auto& inputNode = graph.varNodeConst(staticInputVarNodeIds().at(i));
    if (inputNode.isFixed()) {
      _offset += inputNode.inDomain(bool{true}) ? _coeffs.at(i) : 0;
      indicesToRemove.emplace_back(i);
    }
  }
  for (Int i = static_cast<Int>(indicesToRemove.size()) - 1; i >= 0; --i) {
    removeStaticInputVarNode(
        graph.varNode(staticInputVarNodeIds().at(indicesToRemove.at(i))));
    _coeffs.erase(_coeffs.begin() + indicesToRemove.at(i));
  }
  if (staticInputVarNodeIds().size() == 0) {
    graph.varNode(outputVarNodeIds().front()).fixToValue(_offset);
    setState(InvariantNodeState::SUBSUMED);
  }
}

void BoolLinearNode::registerOutputVars(InvariantGraph& invariantGraph,
                                        propagation::SolverBase& solver) {
  if (_offset != 0) {
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
