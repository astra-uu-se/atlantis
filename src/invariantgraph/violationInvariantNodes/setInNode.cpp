#include "atlantis/invariantgraph/violationInvariantNodes/setInNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/views/inDomain.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/utils/variant.hpp"

namespace atlantis::invariantgraph {

SetInNode::SetInNode(VarNodeId input, std::vector<Int>&& values, VarNodeId r)
    : ViolationInvariantNode({input}, r), _values(std::move(values)) {}

SetInNode::SetInNode(VarNodeId input, std::vector<Int>&& values,
                     bool shouldHold)
    : ViolationInvariantNode({input}, shouldHold), _values(std::move(values)) {}

void SetInNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  ViolationInvariantNode::init(graph, id);
  assert(!isReified() ||
         !graph.varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::all_of(staticInputVarNodeIds().begin(),
                     staticInputVarNodeIds().end(), [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).isIntVar();
                     }));
}

void SetInNode::registerOutputVars(InvariantGraph& graph,
                                   propagation::SolverBase& solver) {
  if (violationVarId(graph) == propagation::NULL_ID) {
    const propagation::VarId input =
        graph.varId(staticInputVarNodeIds().front());
    std::vector<DomainEntry> domainEntries;
    domainEntries.reserve(_values.size());
    std::transform(_values.begin(), _values.end(),
                   std::back_inserter(domainEntries),
                   [](const auto& value) { return DomainEntry(value, value); });

    if (!shouldHold()) {
      assert(!isReified());
      _intermediate = solver.makeIntView<propagation::InDomain>(
          solver, input, std::move(domainEntries));
      setViolationVarId(graph, solver.makeIntView<propagation::NotEqualConst>(
                                   solver, _intermediate, 0));
    } else {
      setViolationVarId(graph, solver.makeIntView<propagation::InDomain>(
                                   solver, input, std::move(domainEntries)));
    }
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void SetInNode::registerNode(InvariantGraph&, propagation::SolverBase&) {}

std::string SetInNode::dotLangIdentifier() const { return "set_in"; }

}  // namespace atlantis::invariantgraph
