#include "atlantis/invariantgraph/violationInvariantNodes/globalCardinalityLowUpClosedNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/globalCardinalityLowUpNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/setInNode.hpp"

namespace atlantis::invariantgraph {

GlobalCardinalityLowUpClosedNode::GlobalCardinalityLowUpClosedNode(
    std::vector<VarNodeId>&& x, std::vector<Int>&& cover,
    std::vector<Int>&& low, std::vector<Int>&& up, VarNodeId r)
    : ViolationInvariantNode({}, std::move(x), r),
      _cover(std::move(cover)),
      _low(std::move(low)),
      _up(std::move(up)) {}

GlobalCardinalityLowUpClosedNode::GlobalCardinalityLowUpClosedNode(
    std::vector<VarNodeId>&& x, std::vector<Int>&& cover,
    std::vector<Int>&& low, std::vector<Int>&& up, bool shouldHold)
    : ViolationInvariantNode({}, std::move(x), shouldHold),
      _cover(std::move(cover)),
      _low(std::move(low)),
      _up(std::move(up)) {}

void GlobalCardinalityLowUpClosedNode::init(InvariantGraph& graph,
                                            const InvariantNodeId& id) {
  ViolationInvariantNode::init(graph, id);
  assert(!isReified() ||
         !graph.varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::all_of(outputVarNodeIds().begin() + 1, outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).isIntVar();
                     }));
  assert(std::all_of(staticInputVarNodeIds().begin(),
                     staticInputVarNodeIds().end(), [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).isIntVar();
                     }));
}

void GlobalCardinalityLowUpClosedNode::registerOutputVars(
    InvariantGraph&, propagation::SolverBase&) {
  throw std::runtime_error("Not implemented");
}

bool GlobalCardinalityLowUpClosedNode::canBeReplaced(
    const InvariantGraph&) const {
  return state() == InvariantNodeState::ACTIVE;
}

bool GlobalCardinalityLowUpClosedNode::replace(InvariantGraph& invariantGraph) {
  if (!isReified() && shouldHold()) {
    invariantGraph.addInvariantNode(
        std::make_unique<GlobalCardinalityLowUpNode>(
            std::vector<VarNodeId>{staticInputVarNodeIds()},
            std::vector<Int>{_cover}, std::vector<Int>{_low},
            std::vector<Int>{_up}));
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

    invariantGraph.addInvariantNode(std::make_unique<IntAllEqualNode>(
        countId, intermediateOutputNodeIds.back(), violationVarNodeIds.back()));
  }

  for (VarNodeId inputId : staticInputVarNodeIds()) {
    violationVarNodeIds.emplace_back(invariantGraph.retrieveBoolVarNode());

    invariantGraph.addInvariantNode(std::make_unique<SetInNode>(
        inputId, std::vector<Int>(_cover), violationVarNodeIds.back()));
  }

  violationVarNodeIds.emplace_back(invariantGraph.retrieveBoolVarNode());

  invariantGraph.addInvariantNode(std::make_unique<GlobalCardinalityLowUpNode>(
      std::vector<VarNodeId>{staticInputVarNodeIds()}, std::vector<Int>{_cover},
      std::vector<Int>{_low}, std::vector<Int>{_up},
      violationVarNodeIds.back()));

  if (isReified()) {
    invariantGraph.addInvariantNode(std::make_unique<ArrayBoolAndNode>(
        std::move(violationVarNodeIds), reifiedViolationNodeId()));
  } else {
    invariantGraph.addInvariantNode(std::make_unique<ArrayBoolAndNode>(
        std::move(violationVarNodeIds), false));
  }

  return true;
}

void GlobalCardinalityLowUpClosedNode::registerNode(InvariantGraph&,
                                                    propagation::SolverBase&) {
  throw std::runtime_error("Not implemented");
}

std::string GlobalCardinalityLowUpClosedNode::dotLangIdentifier() const {
  return "global_cardinality_low_up_closed";
}

}  // namespace atlantis::invariantgraph
