#include "invariantgraph/constraints/intLeNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<IntLeNode> IntLeNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2 ||
      constraint.arguments().size() != 3) {
    throw std::runtime_error("IntLe constraint takes two var bool arguments");
  }

  VariableNode* a = invariantGraph.addVariable(
      std::get<fznparser::IntArg>(constraint.arguments().at(0)));

  VariableNode* b = invariantGraph.addVariable(
      std::get<fznparser::IntArg>(constraint.arguments().at(1)));

  if (constraint.arguments().size() == 2) {
    return std::make_unique<IntLeNode>(a, b, true);
  }

  const auto& reified = get<fznparser::BoolArg>(constraint.arguments().at(2));
  if (std::holds_alternative<bool>(reified)) {
    return std::make_unique<IntLeNode>(a, b, std::get<bool>(reified));
  }
  return std::make_unique<IntLeNode>(
      a, b,
      invariantGraph.addVariable(
          std::get<std::reference_wrapper<const fznparser::BoolVar>>(reified)
              .get()));
}

void IntLeNode::createDefinedVariables(Engine& engine) {
  registerViolation(engine);
}

void IntLeNode::registerWithEngine(Engine& engine) {
  assert(violationVarId() != NULL_ID);

  if (shouldHold()) {
    engine.makeConstraint<LessEqual>(engine, violationVarId(), a()->varId(),
                                     b()->varId());
  } else {
    assert(!isReified());
    engine.makeConstraint<LessThan>(engine, violationVarId(), b()->varId(),
                                    a()->varId());
  }
}

}  // namespace invariantgraph