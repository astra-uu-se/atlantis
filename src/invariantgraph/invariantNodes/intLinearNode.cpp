#include "atlantis/invariantgraph/invariantNodes/intLinearNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/views/intScalarNode.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"
#include "atlantis/propagation/views/scalarView.hpp"

namespace atlantis::invariantgraph {

IntLinearNode::IntLinearNode(std::vector<Int>&& coeffs,
                             std::vector<VarNodeId>&& vars, VarNodeId output,
                             Int offset)
    : InvariantNode({output}, std::move(vars)),
      _coeffs(std::move(coeffs)),
      _offset(offset) {}

void IntLinearNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  InvariantNode::init(graph, id);
  assert(graph.varNodeConst(outputVarNodeIds().front()).isIntVar());
  assert(std::all_of(staticInputVarNodeIds().begin(),
                     staticInputVarNodeIds().end(), [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).isIntVar();
                     }));
}

void IntLinearNode::updateState(InvariantGraph& graph) {
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
    const auto& inputNode = graph.varNodeConst(staticInputVarNodeIds().at(i));
    if (inputNode.isFixed() || _coeffs.at(i) == 0) {
      _offset += _coeffs.at(i) * inputNode.lowerBound();
      indicesToRemove.emplace_back(i);
    }
  }

  for (Int i = indicesToRemove.size() - 1; i >= 0; --i) {
    removeStaticInputVarNode(
        graph.varNode(staticInputVarNodeIds().at(indicesToRemove.at(i))));
    _coeffs.erase(_coeffs.begin() + indicesToRemove.at(i));
  }

  Int lb = _offset;
  Int ub = _offset;
  for (size_t i = 0; i < staticInputVarNodeIds().size(); ++i) {
    const Int v1 = _coeffs.at(i) *
                   graph.varNode(staticInputVarNodeIds().at(i)).lowerBound();
    const Int v2 = _coeffs.at(i) *
                   graph.varNode(staticInputVarNodeIds().at(i)).upperBound();
    lb += std::min(v1, v2);
    ub += std::max(v1, v2);
  }

  auto& outputNode = graph.varNode(outputVarNodeIds().front());

  outputNode.removeValuesBelow(lb);
  outputNode.removeValuesAbove(ub);

  if (staticInputVarNodeIds().empty()) {
    outputNode.fixToValue(_offset);
    setState(InvariantNodeState::SUBSUMED);
  }
}

void IntLinearNode::registerOutputVars(InvariantGraph& invariantGraph,
                                       propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().empty()) {
    return;
  } else if (staticInputVarNodeIds().size() == 1) {
    invariantGraph.varNode(outputVarNodeIds().front())
        .setVarId(solver.makeIntView<propagation::ScalarView>(
            solver, invariantGraph.varId(staticInputVarNodeIds().front()),
            _coeffs.front(), _offset));
    return;
  } else if (_offset == 0) {
    makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
    assert(invariantGraph.varId(outputVarNodeIds().front()).idType ==
           propagation::VarIdType::var);
  } else if (_intermediate == propagation::NULL_ID) {
    _intermediate = solver.makeIntVar(0, 0, 0);
    invariantGraph.varNode(outputVarNodeIds().front())
        .setVarId(solver.makeIntView<propagation::IntOffsetView>(
            solver, _intermediate, _offset));
  }
}

void IntLinearNode::registerNode(InvariantGraph& invariantGraph,
                                 propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);

  std::vector<propagation::VarId> solverVars;
  std::transform(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      std::back_inserter(solverVars), [&](const VarNodeId varNodeId) {
        assert(invariantGraph.varId(varNodeId) != propagation::NULL_ID);
        return invariantGraph.varId(varNodeId);
      });
  solver.makeInvariant<propagation::Linear>(
      solver,
      _intermediate == propagation::NULL_ID
          ? invariantGraph.varId(outputVarNodeIds().front())
          : _intermediate,
      std::vector<Int>(_coeffs), std::move(solverVars));
}

const std::vector<Int>& IntLinearNode::coeffs() const { return _coeffs; }

}  // namespace atlantis::invariantgraph
