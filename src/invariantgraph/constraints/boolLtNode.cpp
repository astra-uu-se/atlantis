#include "invariantgraph/constraints/boolLtNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<BoolLtNode> BoolLtNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2 ||
      constraint.arguments().size() != 3) {
    throw std::runtime_error("BoolLt constraint takes two var bool arguments");
  }
  for (const auto& arg : constraint.arguments()) {
    if (!std::holds_alternative<fznparser::BoolArg>(arg)) {
      throw std::runtime_error(
          "BoolLt constraint takes two var bool arguments");
    }
  }

  VariableNode* a = invariantGraph.addVariable(
      std::get<fznparser::BoolArg>(constraint.arguments().at(0)));

  VariableNode* b = invariantGraph.addVariable(
      std::get<fznparser::BoolArg>(constraint.arguments().at(1)));

  if (constraint.arguments().size() == 2) {
    return std::make_unique<BoolLtNode>(a, b, true);
  }

  const auto& reified = get<fznparser::BoolArg>(constraint.arguments().at(2));
  if (std::holds_alternative<bool>(reified)) {
    return std::make_unique<BoolLtNode>(a, b, std::get<bool>(reified));
  }
  return std::make_unique<BoolLtNode>(
      a, b,
      invariantGraph.addVariable(
          std::get<std::reference_wrapper<const fznparser::BoolVar>>(reified)
              .get()));
}

void BoolLtNode::createDefinedVariables(Engine& engine) {
  registerViolation(engine);
}

void BoolLtNode::registerWithEngine(Engine& engine) {
  assert(violationVarId() != NULL_ID);
  assert(a()->varId() != NULL_ID);
  assert(b()->varId() != NULL_ID);

  if (shouldHold()) {
    engine.makeConstraint<BoolLessThan>(engine, violationVarId(), a()->varId(),
                                        b()->varId());
  } else {
    assert(!isReified());
    engine.makeConstraint<BoolLessEqual>(engine, violationVarId(), b()->varId(),
                                         a()->varId());
  }
}

}  // namespace invariantgraph