#include "atlantis/invariantgraph/views/intScalarNode.hpp"

#include <algorithm>

#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/scalarView.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::invariantgraph {

IntScalarNode::IntScalarNode(InvariantGraph& graph, const VarNodeId staticInput,
                             const VarNodeId output, const Int factor, const Int offset)
    : InvariantNode(graph, {output}, {staticInput}),
      _factor(factor),
      _offset(offset) {}

void IntScalarNode::init(const InvariantNodeId id) {
  InvariantNode::init(id);
  assert(invariantGraphConst()
             .varNodeConst(outputVarNodeIds().front())
             .isIntVar());
  assert(invariantGraph()
             .varNodeConst(staticInputVarNodeIds().front())
             .isIntVar());
}

void IntScalarNode::updateState() {
  if (varNodeConst(input()).isFixed() || outputVarNodeConst(0).isFixed()) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (_factor == 1 && _offset == 0) {
    invariantGraph().replaceVarNode(outputVarNodeIds().front(), staticInputVarNodeIds().front());
    setState(InvariantNodeState::SUBSUMED);
  }
}
bool IntScalarNode::constrainsOutput(VarNodeId) const {
  const Int a = overflow::saturatingAdd(overflow::saturatingMul(staticInputVarNodeConst(0).lowerBound(), _factor), _offset);
  const Int b = overflow::saturatingAdd(overflow::saturatingMul(staticInputVarNodeConst(0).upperBound(), _factor), _offset);
  return !outputVarNodeConst(0).constDomain()->contains(std::min(a, b), std::max(a, b));
}

bool IntScalarNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE || overflow::saturatingAbs(_factor) != 1) {
    return false;
  }

  return varNodeConst(staticInputVarNodeIds().front()).staticInputTo().size() == 1 && varNodeConst(staticInputVarNodeIds().front()).definingNodes().empty() && !varNodeConst(outputVarNodeIds().front()).staticInputTo().empty();
}

bool IntScalarNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  invariantGraph().addInvariantNode(std::make_shared<IntScalarNode>(invariantGraph(), outputVarNodeIds().front(), staticInputVarNodeIds().front(), _factor, overflow::saturatingSub(0, _offset)));
  return true;
}

void IntScalarNode::registerOutputVars(propagation::SolverBase& solver,
                                       SolverMapping& mapping) const {
  if (mapping.solverId(outputVarNodeIds().front()) == propagation::NULL_ID) {
    mapping.setSolverId(
        outputVarNodeIds().front(),
        solver.makeIntView<propagation::ScalarView>(
            solver, mapping.solverId(input()), _factor, _offset));
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void IntScalarNode::registerNode(propagation::SolverBase&,
                                 SolverMapping&) const {}

std::ostream& IntScalarNode::dotLangEntry(std::ostream& o) const { return o; }

std::ostream& IntScalarNode::dotLangEdges(std::ostream& o) const {
  const std::string label =
      (_factor == 1 ? "" : ("* " + std::to_string(_factor))) +
      (_factor != 1 && _offset != 0 ? " " : "") +
      (_offset == 0
           ? ""
           : ((_offset < 0 ? "- " : "+ ") + std::to_string(std::abs(_offset))));

  return o << staticInputVarNodeIds().front() << " -> "
           << outputVarNodeIds().front() << "[label=\"" << label << "\"];"
           << std::endl;
}

std::string IntScalarNode::dotLangIdentifier() const { return ""; }

}  // namespace atlantis::invariantgraph
