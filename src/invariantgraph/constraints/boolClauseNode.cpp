#include "invariantgraph/constraints/boolClauseNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::BoolClauseNode>
invariantgraph::BoolClauseNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  std::vector<invariantgraph::VariableNode*> as =
      mappedVariableVector(model, constraint.arguments[0], variableMap);
  std::vector<invariantgraph::VariableNode*> bs =
      mappedVariableVector(model, constraint.arguments[1], variableMap);

  if (constraint.arguments.size() >= 3) {
    if (std::holds_alternative<bool>(constraint.arguments[2])) {
      auto shouldHold = std::get<bool>(constraint.arguments[2]);
      return std::make_unique<invariantgraph::BoolClauseNode>(as, bs,
                                                              shouldHold);
    } else {
      auto r = mappedVariable(constraint.arguments[2], variableMap);
      return std::make_unique<invariantgraph::BoolClauseNode>(as, bs, r);
    }
  }
  return std::make_unique<BoolClauseNode>(as, bs, true);
}

void invariantgraph::BoolClauseNode::createDefinedVariables(Engine& engine) {
  if (violationVarId() == NULL_ID) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(engine.makeIntView<EqualConst>(
          _sumVarId,
          static_cast<Int>(_as.size()) + static_cast<Int>(_bs.size())));
    } else {
      assert(!isReified());
      setViolationVarId(engine.makeIntView<NotEqualConst>(
          _sumVarId,
          static_cast<Int>(_as.size()) + static_cast<Int>(_bs.size())));
    }
  }
}

void invariantgraph::BoolClauseNode::registerWithEngine(Engine& engine) {
  std::vector<VarId> inputs;
  inputs.reserve(_as.size() + _bs.size());
  std::transform(_as.begin(), _as.end(), std::back_inserter(inputs),
                 [&](const auto& var) { return var->inputVarId(); });

  std::transform(
      _bs.begin(), _bs.end(), std::back_inserter(inputs), [&](const auto& var) {
        return engine.makeIntView<NotEqualConst>(var->inputVarId(), 0);
      });

  assert(std::all_of(inputs.begin(), inputs.end(),
                     [&](const VarId varId) { return varId != NULL_ID; }));

  assert(_sumVarId != NULL_ID);
  assert(violationVarId() != NULL_ID);
  engine.makeInvariant<BoolLinear>(_sumVarId, inputs);
}
