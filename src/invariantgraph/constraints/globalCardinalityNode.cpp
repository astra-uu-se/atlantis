#include "invariantgraph/constraints/globalCardinalityNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::GlobalCardinalityNode>
invariantgraph::GlobalCardinalityNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert((constraint.name == "fzn_global_cardinality" &&
          constraint.arguments.size() == 3) ||
         (constraint.name == "fzn_global_cardinality_reif" &&
          constraint.arguments.size() == 4));

  auto x = mappedVariableVector(model, constraint.arguments[0], variableMap);

  auto cover = integerVector(model, constraint.arguments[1]);

  auto counts =
      mappedVariableVector(model, constraint.arguments[2], variableMap);

  assert(cover.size() == counts.size());

  bool shouldHold = true;
  VariableNode* r = nullptr;

  if (constraint.arguments.size() >= 4) {
    if (std::holds_alternative<bool>(constraint.arguments[3])) {
      shouldHold = std::get<bool>(constraint.arguments[3]);
    } else {
      r = mappedVariable(constraint.arguments[3], variableMap);
    }
  }

  if (r != nullptr) {
    return std::make_unique<GlobalCardinalityNode>(x, cover, counts, r);
  }
  assert(r == nullptr);
  return std::make_unique<GlobalCardinalityNode>(x, cover, counts, shouldHold);
}

void invariantgraph::GlobalCardinalityNode::createDefinedVariables(
    Engine& engine) {
  if (!isReified() && shouldHold()) {
    for (auto* const definedNode : definedVariables()) {
      if (definedNode->varId() == NULL_ID) {
        definedNode->setVarId(engine.makeIntVar(0, 0, numInputs()));
      }
    }
  } else if (_intermediate.size() == 0) {
    for (auto iter = countsBegin(); iter != countsEnd(); ++iter) {
      _intermediate.emplace_back(engine.makeIntVar(0, 0, numInputs()));
      if (_cover.size() > 1) {
        _violations.emplace_back(engine.makeIntVar(0, 0, numInputs()));
      }
    }
    registerViolation(engine);
  }
}

void invariantgraph::GlobalCardinalityNode::registerWithEngine(Engine& engine) {
  std::vector<VarId> inputs;
  std::transform(inputsBegin(), inputsEnd(), std::back_inserter(inputs),
                 [&](auto node) { return node->varId(); });

  if (!isReified() && shouldHold()) {
    std::vector<VarId> outputs;
    std::transform(definedVariables().begin(), definedVariables().end(),
                   std::back_inserter(outputs),
                   [&](auto node) { return node->varId(); });
    engine.makeInvariant<GlobalCardinalityOpen>(inputs, _cover, outputs);
  } else {
    assert(violationVarId() != NULL_ID);
    engine.makeInvariant<GlobalCardinalityOpen>(inputs, _cover, _intermediate);
    size_t i = 0;
    if (_cover.size() == 1) {
      if (shouldHold()) {
        engine.makeConstraint<Equal>(violationVarId(), _intermediate.front(),
                                     (*countsBegin())->varId());
      } else {
        engine.makeConstraint<NotEqual>(violationVarId(), _intermediate.front(),
                                        (*countsBegin())->varId());
      }
    } else {
      for (auto iter = countsBegin(); iter != countsEnd(); ++iter) {
        if (shouldHold()) {
          engine.makeConstraint<Equal>(_violations.at(i), _intermediate.at(i),
                                       (*iter)->varId());
        } else {
          engine.makeConstraint<NotEqual>(
              _violations.at(i), _intermediate.at(i), (*iter)->varId());
        }
        ++i;
      }
      if (shouldHold()) {
        engine.makeInvariant<Linear>(_violations, violationVarId());
      } else {
        engine.makeInvariant<Exists>(_violations, violationVarId());
      }
    }
  }
}
