#include "atlantis/invariantgraph/invariantNodes/intLinearNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/views/intScalarNode.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"

namespace atlantis::invariantgraph {

IntLinearNode::IntLinearNode(std::vector<Int>&& coeffs,
                             std::vector<VarNodeId>&& vars, VarNodeId output)
    : InvariantNode({output}, std::move(vars)), _coeffs(std::move(coeffs)) {}

void IntLinearNode::registerOutputVars(InvariantGraph& invariantGraph,
                                       propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

bool IntLinearNode::canBeReplaced(const InvariantGraph& invariantGraph) const {
  size_t numFixed = 0;
  for (const auto& input : staticInputVarNodeIds()) {
    numFixed +=
        static_cast<size_t>(invariantGraph.varNodeConst(input).isFixed());
    if (numFixed > 1) {
      return false;
    }
  }
  return true;
}

bool IntLinearNode::replace(InvariantGraph& invariantGraph) {
  if (!canBeReplaced(invariantGraph)) {
    return false;
  }
  if (_coeffs.front() == 1 && _offset == 0) {
    invariantGraph.replaceVarNode(outputVarNodeIds().front(),
                                  staticInputVarNodeIds().front());
    deactivate(invariantGraph);
  } else {
    invariantGraph.addInvariantNode(std::make_unique<IntScalarNode>(
        staticInputVarNodeIds().front(), outputVarNodeIds().front(),
        _coeffs.front(), _offset));
  }
  return true;
}

void IntLinearNode::registerNode(InvariantGraph& invariantGraph,
                                 propagation::SolverBase& solver) {
  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);

  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const VarNodeId varNodeId) {
                   return invariantGraph.varId(varNodeId);
                 });
  solver.makeInvariant<propagation::Linear>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      std::vector<Int>(_coeffs), std::move(solverVars));
}

const std::vector<Int>& IntLinearNode::coeffs() const { return _coeffs; }

}  // namespace atlantis::invariantgraph
