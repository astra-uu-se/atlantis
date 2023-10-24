#include "invariantgraph/violationInvariantNodes/setInNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

SetInNode::SetInNode(VarNodeId input, std::vector<Int>&& values, VarNodeId r)
    : ViolationInvariantNode({input}, r), _values(std::move(values)) {}

SetInNode::SetInNode(VarNodeId input, std::vector<Int>&& values,
                     bool shouldHold)
    : ViolationInvariantNode({input}, shouldHold), _values(std::move(values)) {}

std::unique_ptr<SetInNode> SetInNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const VarNodeId varNodeId = invariantGraph.createVarNode(
      std::get<fznparser::IntArg>(constraint.arguments().at(0)));

  fznparser::IntSet valueSet =
      std::get<fznparser::IntSetArg>(constraint.arguments().at(1))
          .toParameter();

  // Note: if the valueSet is an IntRange, here all the values are collected
  // into a vector. If it turns out memory usage is an issue, this should be
  // mitigated.

  std::vector<Int> values = valueSet.populateElements();

  if (constraint.arguments().size() == 2) {
    return std::make_unique<SetInNode>(std::move(varNodeId), std::move(values),
                                       true);
  }
  const fznparser::BoolArg reified =
      std::get<fznparser::BoolArg>(constraint.arguments().at(2));

  if (reified.isFixed()) {
    return std::make_unique<SetInNode>(std::move(varNodeId), std::move(values),
                                       reified.toParameter());
  }
  return std::make_unique<SetInNode>(
      std::move(varNodeId), std::move(values),
      invariantGraph.createVarNode(reified.var()));
}

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