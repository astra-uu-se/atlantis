#include "invariantgraph/violationInvariantNodes/setInNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<SetInNode> SetInNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto variable = invariantGraph.createVarNode(
      std::get<fznparser::IntArg>(constraint.arguments().at(0)));

  fznparser::IntSet valueSet =
      std::get<fznparser::IntSetArg>(constraint.arguments().at(1))
          .toParameter();

  // Note: if the valueSet is an IntRange, here all the values are collected
  // into a vector. If it turns out memory usage is an issue, this should be
  // mitigated.

  std::vector<Int> values = valueSet.populateElements();

  if (constraint.arguments().size() == 2) {
    return std::make_unique<SetInNode>(std::move(variable), std::move(values),
                                       true);
  }
  const fznparser::BoolArg reified =
      std::get<fznparser::BoolArg>(constraint.arguments().at(2));

  if (reified.isFixed()) {
    return std::make_unique<SetInNode>(std::move(variable), std::move(values),
                                       reified.toParameter());
  }
  return std::make_unique<SetInNode>(
      std::move(variable), std::move(values),
      invariantGraph.createVarNode(reified.var()));
}

void SetInNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                        Engine& engine) {
  if (violationVarId() == NULL_ID) {
    const VarId input = staticInputVarNodeIds().front()->varId();
    std::vector<DomainEntry> domainEntries;
    domainEntries.reserve(_values.size());
    std::transform(_values.begin(), _values.end(),
                   std::back_inserter(domainEntries),
                   [](const auto& value) { return DomainEntry(value, value); });

    if (!shouldHold()) {
      assert(!isReified());
      _intermediate =
          engine.makeIntView<InDomain>(engine, input, std::move(domainEntries));
      setViolationVarId(
          engine.makeIntView<NotEqualConst>(engine, _intermediate, 0));
    } else {
      setViolationVarId(engine.makeIntView<InDomain>(engine, input,
                                                     std::move(domainEntries)));
    }
  }
}

void SetInNode::registerNode(InvariantGraph& invariantGraph, Engine&) {}

}  // namespace invariantgraph