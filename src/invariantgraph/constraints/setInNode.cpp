#include "invariantgraph/constraints/setInNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<SetInNode> SetInNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto variable = invariantGraph.addVariable(
      std::get<fznparser::IntArg>(constraint.arguments().at(0)));

  fznparser::IntSet valueSet =
      std::get<fznparser::IntSetArg>(constraint.arguments().at(1))
          .toParameter();

  // Note: if the valueSet is an IntRange, here all the values are collected
  // into a vector. If it turns out memory usage is an issue, this should be
  // mitigated.

  std::vector<Int> values = valueSet.populateElements();

  if (constraint.arguments().size() == 2) {
    return std::make_unique<SetInNode>(variable, values, true);
  }
  const fznparser::BoolArg reified =
      std::get<fznparser::BoolArg>(constraint.arguments().at(2));

  if (std::holds_alternative<bool>(reified)) {
    return std::make_unique<SetInNode>(variable, values,
                                       std::get<bool>(reified));
  }
  return std::make_unique<SetInNode>(
      variable, values,
      invariantGraph.addVariable(
          std::get<std::reference_wrapper<const fznparser::BoolVar>>(reified)
              .get()));
}

void SetInNode::createDefinedVariables(Engine& engine) {
  if (violationVarId() == NULL_ID) {
    const VarId input = staticInputs().front()->varId();
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

void SetInNode::registerWithEngine(Engine&) {}

}  // namespace invariantgraph