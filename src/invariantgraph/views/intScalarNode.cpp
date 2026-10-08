#include "atlantis/invariantgraph/views/intScalarNode.hpp"

#include <algorithm>

#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/scalarView.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::invariantgraph {

IntScalarNode::IntScalarNode(InvariantGraph& graph, VarNode& staticInput,
                             VarNode& output, const Int factor,
                             const Int offset)
    : InvariantNode(graph, {output.ptr()}, {staticInput.ptr()}),
      _factor(factor),
      _offset(offset) {}

void IntScalarNode::init() {
  InvariantNode::init();
  assert(
      outputVarNode(0).isIntVar());
  assert(
      staticInputVarNode(0).isIntVar());
}

void IntScalarNode::updateState() {
  if (input().isFixed() || outputVarNode(0).isFixed()) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (_factor == 1 && _offset == 0) {
    invariantGraph().replaceVarNode(outputVarNode(0),
                                    staticInputVarNode(0));
    setState(InvariantNodeState::SUBSUMED);
  }
}
bool IntScalarNode::constrainsOutput(const VarNode&) const {
  const Int a = overflow::saturatingAdd(
      overflow::saturatingMul(staticInputVarNode(0).lowerBound(),
                              _factor),
      _offset);
  const Int b = overflow::saturatingAdd(
      overflow::saturatingMul(staticInputVarNode(0).upperBound(),
                              _factor),
      _offset);
  return !outputVarNode(0).constDomain()->contains(std::min(a, b),
                                                           std::max(a, b));
}

bool IntScalarNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE ||
      overflow::saturatingAbs(_factor) != 1) {
    return false;
  }

  return staticInputVarNode(0).staticInputTo().size() ==
             1 &&
         staticInputVarNode(0).definingNodes().empty() &&
         !outputVarNode(0).staticInputTo().empty();
}

bool IntScalarNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  invariantGraph().addInvariantNode(std::make_shared<IntScalarNode>(
      invariantGraph(), outputVarNode(0), staticInputVarNode(0),
      _factor, overflow::saturatingSub(0, _offset)));
  return true;
}

void IntScalarNode::registerOutputVars(propagation::SolverBase& solver,
                                       SolverMapping& mapping) const {
  if (mapping.solverId(outputVarNode(0)) == propagation::NULL_ID) {
    mapping.setSolverId(
        outputVarNode(0),
        solver.makeIntView<propagation::ScalarView>(
            solver, mapping.solverId(input()), _factor, _offset));
  }
  assert(std::ranges::all_of(outputVarNodes().begin(), outputVarNodes().end(),
                             [&](const std::shared_ptr<VarNode>& vId) {
                               return mapping.solverId(vId) !=
                                      propagation::NULL_ID;
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

  return o << reinterpret_cast<size_t>(&staticInputVarNode(0)) << " -> "
           << reinterpret_cast<size_t>(&outputVarNode(0)) << "[label=\"" << label << "\"];"
           << std::endl;
}

std::string IntScalarNode::dotLangIdentifier() const { return ""; }

}  // namespace atlantis::invariantgraph
