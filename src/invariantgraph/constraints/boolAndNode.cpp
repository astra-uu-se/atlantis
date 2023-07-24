#include "invariantgraph/constraints/boolAndNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::BoolAndNode>
invariantgraph::BoolAndNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2 ||
      constraint.arguments().size() != 3) {
    throw std::runtime_error("BoolAnd constraint takes two var bool arguments");
  }
  for (const auto& arg : constraint.arguments()) {
    if (!std::holds_alternative<fznparser::BoolArg>(arg)) {
      throw std::runtime_error(
          "BoolAnd constraint takes two var bool arguments");
    }
  }
  const auto& a = invariantGraph.addVariable(
      std::get<fznparser::BoolArg>(constraint.arguments().at(0)));

  const auto& b = invariantGraph.addVariable(
      std::get<fznparser::BoolArg>(constraint.arguments().at(1)));

  if (constraint.arguments().size() == 2) {
    return std::make_unique<BoolAndNode>(a, b, true);
  }

  const auto& reified = get<fznparser::BoolArg>(constraint.arguments().at(2));
  if (std::holds_alternative<bool>(reified)) {
    return std::make_unique<invariantgraph::BoolAndNode>(
        a, b, std::get<bool>(reified));
  }
  return std::make_unique<invariantgraph::BoolAndNode>(
      a, b,
      invariantGraph.addVariable(
          std::get<std::reference_wrapper<const fznparser::BoolVar>>(reified)
              .get()));
}

void invariantgraph::BoolAndNode::createDefinedVariables(Engine& engine) {
  if (violationVarId() == NULL_ID) {
    if (shouldHold()) {
      registerViolation(engine);
    } else {
      assert(!isReified());
      _intermediate = engine.makeIntVar(0, 0, 0);
      setViolationVarId(
          engine.makeIntView<NotEqualConst>(engine, _intermediate, 0));
    }
  }
}

void invariantgraph::BoolAndNode::registerWithEngine(Engine& engine) {
  assert(violationVarId() != NULL_ID);
  assert(a()->varId() != NULL_ID);
  assert(b()->varId() != NULL_ID);

  engine.makeInvariant<BoolAnd>(engine, violationVarId(), a()->varId(),
                                b()->varId());
}