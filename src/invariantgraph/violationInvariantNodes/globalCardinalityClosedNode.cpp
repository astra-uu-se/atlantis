#include "atlantis/invariantgraph/violationInvariantNodes/globalCardinalityClosedNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/globalCardinalityNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/intCountNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/setInNode.hpp"

namespace atlantis::invariantgraph {

GlobalCardinalityClosedNode::GlobalCardinalityClosedNode(
    InvariantGraph& graph, std::vector<VarNodeId>&& inputs,
    std::vector<Int>&& cover, std::vector<VarNodeId>&& counts, bool shouldHold)
    : ViolationInvariantNode(graph, std::move(counts), std::move(inputs),
                             shouldHold),
      _cover(std::move(cover)) {}

void GlobalCardinalityClosedNode::init(const InvariantNodeId& id) {
  ViolationInvariantNode::init(id);
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
  return state() == InvariantNodeState::ACTIVE;
}

bool GlobalCardinalityClosedNode::replace(InvariantGraph& invariantGraph) {
  if (!isReified() && shouldHold()) {
    invariantGraph.addInvariantNode(std::make_shared<GlobalCardinalityNode>(
        graph, std::vector<VarNodeId>{staticInputVarNodeIds()},
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

    invariantGraph.addInvariantNode(std::make_shared<IntAllEqualNode>(
        graph, countId, intermediateOutputNodeIds.back(),
        violationVarNodeIds.back()));
  }

  for (VarNodeId inputId : staticInputVarNodeIds()) {
    violationVarNodeIds.emplace_back(invariantGraph.retrieveBoolVarNode());

    invariantGraph.addInvariantNode(std::make_shared<SetInNode>(
        graph, inputId, std::vector<Int>(_cover), violationVarNodeIds.back()));
  }

  invariantGraph.addInvariantNode(std::make_shared<GlobalCardinalityNode>(
      graph, std::vector<VarNodeId>{staticInputVarNodeIds()},
      std::vector<Int>{_cover}, std::move(intermediateOutputNodeIds)));

  if (isReified()) {
    invariantGraph.addInvariantNode(std::make_shared<ArrayBoolAndNode>(
        graph, std::move(violationVarNodeIds), reifiedViolationNodeId()));
    return true;
  } else {
    invariantGraph.addInvariantNode(std::make_shared<ArrayBoolAndNode>(
        graph, std::move(violationVarNodeIds), false));
    return true;
  }
  return true;
}

void GlobalCardinalityClosedNode::registerNode(InvariantGraph&,
                                               propagation::SolverBase&) {
  throw std::runtime_error("Not implemented");
}

}  // namespace atlantis::invariantgraph
