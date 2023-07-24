#include "invariantgraph/constraints/boolEqNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<BoolEqNode> BoolEqNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2 ||
      constraint.arguments().size() != 3) {
    throw std::runtime_error("BoolEq constraint takes two var bool arguments");
  }
  for (const auto& arg : constraint.arguments()) {
    if (!std::holds_alternative<fznparser::BoolArg>(arg)) {
      throw std::runtime_error(
          "BoolEq constraint takes two var bool arguments");
    }
  }

  VariableNode* a = invariantGraph.addVariable(
      std::get<fznparser::BoolArg>(constraint.arguments().at(0)));

  VariableNode* b = invariantGraph.addVariable(
      std::get<fznparser::BoolArg>(constraint.arguments().at(1)));

  if (constraint.arguments().size() == 2) {
    return std::make_unique<BoolEqNode>(a, b, true);
  }

  const auto& reified = get<fznparser::BoolArg>(constraint.arguments().at(2));
  if (std::holds_alternative<bool>(reified)) {
    return std::make_unique<BoolEqNode>(a, b, std::get<bool>(reified));
  }
  return std::make_unique<BoolEqNode>(
      a, b,
      invariantGraph.addVariable(
          std::get<std::reference_wrapper<const fznparser::BoolVar>>(reified)
              .get()));
}

void BoolEqNode::createDefinedVariables(Engine& engine) {
  registerViolation(engine);
}

void BoolEqNode::registerWithEngine(Engine& engine) {
  assert(violationVarId() != NULL_ID);
  assert(a()->varId() != NULL_ID);
  assert(b()->varId() != NULL_ID);

  if (shouldHold()) {
    engine.makeConstraint<BoolEqual>(engine, violationVarId(), a()->varId(),
                                     b()->varId());
  } else {
    engine.makeInvariant<BoolXor>(engine, violationVarId(), a()->varId(),
                                  b()->varId());
  }
}

}  // namespace invariantgraph