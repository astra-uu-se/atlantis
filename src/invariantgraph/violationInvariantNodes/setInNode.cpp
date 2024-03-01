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

void SetInNode::registerOutputVars(InvariantGraph& invariantGraph,
                                   propagation::SolverBase& solver) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    const propagation::VarId input =
        invariantGraph.varId(staticInputVarNodeIds().front());
    std::vector<DomainEntry> domainEntries;
    domainEntries.reserve(_values.size());
    std::transform(_values.begin(), _values.end(),
                   std::back_inserter(domainEntries),
                   [](const auto& value) { return DomainEntry(value, value); });

    if (!shouldHold()) {
      assert(!isReified());
      _intermediate = solver.makeIntView<propagation::InDomain>(
          solver, input, std::move(domainEntries));
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::NotEqualConst>(
                            solver, _intermediate, 0));
    } else {
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::InDomain>(
                            solver, input, std::move(domainEntries)));
    }
  }
}

void SetInNode::registerNode(InvariantGraph&, propagation::SolverBase&) {}

}  // namespace atlantis::invariantgraph
