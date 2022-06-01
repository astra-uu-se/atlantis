#include "invariantgraph/constraints/globalCardinalityClosedNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::GlobalCardinalityClosedNode>
invariantgraph::GlobalCardinalityClosedNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

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
    return std::make_unique<GlobalCardinalityClosedNode>(x, cover, counts, r);
  }
  assert(r == nullptr);
  return std::make_unique<GlobalCardinalityClosedNode>(x, cover, counts,
                                                       shouldHold);
}

void invariantgraph::GlobalCardinalityClosedNode::createDefinedVariables(
    Engine& engine) {
  if (violationVarId() == NULL_ID) {
    registerViolation(engine);
    if (!isReified() && shouldHold()) {
      for (auto* const countOutput : definedVariables()) {
        if (countOutput->varId() == NULL_ID) {
          countOutput->setVarId(engine.makeIntVar(0, 0, _inputs.size()));
        }
      }
    } else {
      for (size_t i = 0; i < _counts.size(); ++i) {
        _intermediate.emplace_back(engine.makeIntVar(0, 0, _inputs.size()));
      }
      for (size_t i = 0; i < _counts.size(); ++i) {
        _violations.emplace_back(engine.makeIntVar(0, 0, _inputs.size()));
      }
      // intermediate viol for GCC closed constraint
      if (!shouldHold()) {
        _shouldFailViol = engine.makeIntVar(0, 0, _inputs.size());
        _violations.emplace_back(
            engine.makeIntView<NotEqualView>(_shouldFailViol, 0));
      } else {
        _violations.emplace_back(engine.makeIntVar(0, 0, _inputs.size()));
      }
    }
  }
}

void invariantgraph::GlobalCardinalityClosedNode::registerWithEngine(
    Engine& engine) {
  assert(violationVarId() != NULL_ID);

  std::vector<VarId> inputs;
  std::transform(_inputs.begin(), _inputs.end(), std::back_inserter(inputs),
                 [&](auto node) { return node->varId(); });

  if (!isReified() && shouldHold()) {
    assert(_intermediate.size() == 0);
    assert(_violations.size() == 0);
    std::vector<VarId> countOutputs;
    std::transform(_counts.begin(), _counts.end(),
                   std::back_inserter(countOutputs),
                   [&](auto node) { return node->varId(); });

    engine.makeInvariant<GlobalCardinalityClosed>(violationVarId(), inputs,
                                                  _cover, countOutputs);
  } else {
    assert(_intermediate.size() == _counts.size());
    assert(_violations.size() == _counts.size() + 1);
    if (isReified()) {
      engine.makeInvariant<GlobalCardinalityClosed>(_violations.back(), inputs,
                                                    _cover, _intermediate);
    } else {
      assert(!shouldHold());
      engine.makeInvariant<GlobalCardinalityClosed>(_shouldFailViol, inputs,
                                                    _cover, _intermediate);
    }
    for (size_t i = 0; i < _counts.size(); ++i) {
      if (shouldHold()) {
        engine.makeConstraint<Equal>(_violations.at(i), _intermediate.at(i),
                                     _counts.at(i)->varId());
      } else {
        engine.makeConstraint<NotEqual>(_violations.at(i), _intermediate.at(i),
                                        _counts.at(i)->varId());
      }
    }
    if (shouldHold()) {
      // To hold, each count must be equal to its corresponding intermediate:
      engine.makeInvariant<Linear>(_violations, violationVarId());
    } else {
      // To hold, only one count must not be equal to its corresponding
      // intermediate:
      engine.makeInvariant<Exists>(_violations, violationVarId());
    }
  }
}
