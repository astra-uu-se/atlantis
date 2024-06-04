#include "atlantis/invariantgraph/violationInvariantNodes/globalCardinalityClosedNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/globalCardinalityNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/intCountNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intEqNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/setInNode.hpp"

namespace atlantis::invariantgraph {

GlobalCardinalityClosedNode::GlobalCardinalityClosedNode(
    std::vector<VarNodeId>&& inputs, std::vector<Int>&& cover,
    std::vector<VarNodeId>&& counts, bool shouldHold)
    : ViolationInvariantNode(std::move(counts), std::move(inputs), shouldHold),
      _cover(std::move(cover)) {}

GlobalCardinalityClosedNode::GlobalCardinalityClosedNode(
    std::vector<VarNodeId>&& inputs, std::vector<Int>&& cover,
    std::vector<VarNodeId>&& counts, VarNodeId r)
    : ViolationInvariantNode(std::move(counts), std::move(inputs), r),
      _cover(std::move(cover)) {}

void GlobalCardinalityClosedNode::registerOutputVars(InvariantGraph&,
                                                     propagation::SolverBase&) {
  throw std::runtime_error("Not implemented");
}

bool GlobalCardinalityClosedNode::canBeReplaced(const InvariantGraph&) const {
  return true;
}

bool GlobalCardinalityClosedNode::replace(InvariantGraph& invariantGraph) {
  if (!isReified() && shouldHold()) {
    invariantGraph.addInvariantNode(std::make_unique<GlobalCardinalityNode>(
        std::vector<VarNodeId>{staticInputVarNodeIds()},
        std::vector<Int>{_cover}, std::vector<VarNodeId>{outputVarNodeIds()}));
    return true;
  }

  std::vector<VarNodeId> violationVarNodeIds;
  violationVarNodeIds.reserve(staticInputVarNodeIds().size() +
                              outputVarNodeIds().size() + 1);

  std::vector<VarNodeId> intermediateOutputNodeIds;
  intermediateOutputNodeIds.reserve(outputVarNodeIds().size());

  for (VarNodeId countId : outputVarNodeIds()) {
    intermediateOutputNodeIds.emplace_back(invariantGraph.retrieveIntVarNode(
        SearchDomain(0, static_cast<Int>(staticInputVarNodeIds().size())),
        VarNode::DomainType::NONE));

    violationVarNodeIds.emplace_back(invariantGraph.retrieveBoolVarNode());

    invariantGraph.addInvariantNode(std::make_unique<IntEqNode>(
        countId, intermediateOutputNodeIds.back(), violationVarNodeIds.back()));
  }

  for (VarNodeId inputId : staticInputVarNodeIds()) {
    violationVarNodeIds.emplace_back(invariantGraph.retrieveBoolVarNode());

    invariantGraph.addInvariantNode(std::make_unique<SetInNode>(
        inputId, std::vector<Int>(_cover), violationVarNodeIds.back()));
  }

  invariantGraph.addInvariantNode(std::make_unique<GlobalCardinalityNode>(
      std::vector<VarNodeId>{staticInputVarNodeIds()}, std::vector<Int>{_cover},
      std::move(intermediateOutputNodeIds)));

  if (isReified()) {
    invariantGraph.addInvariantNode(std::make_unique<ArrayBoolAndNode>(
        std::move(violationVarNodeIds), reifiedViolationNodeId()));
    return true;
  } else {
    invariantGraph.addInvariantNode(std::make_unique<ArrayBoolAndNode>(
        std::move(violationVarNodeIds), false));
    return true;
  }
  return true;
}

void GlobalCardinalityClosedNode::registerNode(InvariantGraph&,
                                               propagation::SolverBase&) {
  throw std::runtime_error("Not implemented");
}

}  // namespace atlantis::invariantgraph
