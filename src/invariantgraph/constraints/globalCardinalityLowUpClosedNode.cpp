#include "invariantgraph/constraints/globalCardinalityLowUpClosedNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::GlobalCardinalityLowUpClosedNode>
invariantgraph::GlobalCardinalityLowUpClosedNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto x = mappedVariableVector(model, constraint.arguments[0], variableMap);

  auto cover = integerVector(model, constraint.arguments[1]);

  auto low = integerVector(model, constraint.arguments[2]);
  auto up = integerVector(model, constraint.arguments[3]);

  if (cover.size() != low.size() || cover.size() != up.size()) {
    throw BadArgumentError(
        "GlobalCardinalityLowUpClosedNode: cover, low counts, and up counts "
        "must have the same sizes.");
  }

  for (size_t i = 0; i < cover.size(); ++i) {
    if (up[i] < 0 || up[i] < low[i]) {
      throw BadArgumentError(
          "GlobalCardinalityLowUpClosedNode: Could not create " +
          constraint.name +
          ". up counts must be 0 or greater and low counts "
          "must be less than up counts");
    }
    low[i] = std::max<Int>(0, low[i]);
  }

  assert(cover.size() == low.size());
  assert(cover.size() == up.size());

  std::vector<Int> filteredCover;
  std::vector<Int> filteredLow;
  std::vector<Int> filteredUp;
  filteredCover.reserve(cover.size());
  filteredLow.reserve(low.size());
  filteredUp.reserve(up.size());
  for (size_t i = 0; i < cover.size(); ++i) {
    if (up[i] != 0) {
      filteredCover.emplace_back(cover[i]);
      filteredLow.emplace_back(low[i]);
      filteredUp.emplace_back(up[i]);
    }
  }

  bool shouldHold = true;
  VariableNode* r = nullptr;

  if (constraint.arguments.size() >= 5) {
    if (std::holds_alternative<bool>(constraint.arguments[4])) {
      shouldHold = std::get<bool>(constraint.arguments[4]);
    } else {
      r = mappedVariable(constraint.arguments[4], variableMap);
    }
  }

  if (r != nullptr) {
    return std::make_unique<GlobalCardinalityLowUpClosedNode>(
        x, filteredCover, filteredLow, filteredUp, r);
  }
  return std::make_unique<GlobalCardinalityLowUpClosedNode>(
      x, filteredCover, filteredLow, filteredUp, shouldHold);
}

void invariantgraph::GlobalCardinalityLowUpClosedNode::createDefinedVariables(
    Engine& engine) {
  if (violationVarId() == NULL_ID) {
    if (!shouldHold()) {
      _intermediate = engine.makeIntVar(0, 0, staticInputs().size());
      setViolationVarId(engine.makeIntView<NotEqualConst>(_intermediate, 0));
    } else {
      registerViolation(engine);
    }
  }
}

bool invariantgraph::GlobalCardinalityLowUpClosedNode::prune() {
  if (isReified() || !shouldHold()) {
    return false;
  }
  std::vector<Int> sortedCover(_cover);
  std::sort(sortedCover.begin(), sortedCover.end());

  std::vector<std::unordered_set<VariableNode*>> inCover(_cover.size());
  std::vector<bool> isCoverFixed(_cover.size(), false);
  bool didChange = false;
  for (auto* var : staticInputs()) {
    const size_t prevDomSize = var->domain().size();
    var->domain().removeBelow(sortedCover.front());
    var->domain().removeAbove(sortedCover.back());
    didChange = didChange || var->domain().size() != prevDomSize;
  }
  size_t i = 0;
  for (Int val = sortedCover.front(); val <= sortedCover.back(); ++val) {
    while (sortedCover[i] < val) {
      ++i;
    }
    assert(i < sortedCover.size());
    if (val != sortedCover[i]) {
      for (auto* var : staticInputs()) {
        var->domain().removeValue(val);
      }
    }
  }
  for (size_t j = 0; j < _cover.size(); ++j) {
    for (auto* var : staticInputs()) {
      if (var->domain().contains(_cover[j])) {
        inCover[j].emplace(var);
      }
    }
    isCoverFixed[j] = std::all_of(
        inCover[j].begin(), inCover[j].end(),
        ([&](auto* const var) { return var->domain().isConstant(); }));
  }
  for (size_t j = 0; j < _cover.size(); ++j) {
    assert(_low[j] >= 0);
    if (_low[j] != static_cast<Int>(inCover[j].size()) || isCoverFixed[j]) {
      continue;
    }
    for (auto* var : inCover[j]) {
      if (!var->domain().isConstant()) {
        var->domain().fix(_cover[j]);
        didChange = true;
      }
    }
    isCoverFixed[j] = true;
    for (size_t k = 0; k < _cover.size(); ++k) {
      if (j != k && !isCoverFixed[k]) {
        for (auto* const var : inCover[j]) {
          inCover[k].erase(var);
        }
      }
    }
    if (j > 0) {
      j = 0;
    }
  }
  return didChange;
}

void invariantgraph::GlobalCardinalityLowUpClosedNode::registerWithEngine(
    Engine& engine) {
  std::vector<VarId> inputs;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(inputs),
                 [&](auto node) { return node->varId(); });

  if (shouldHold()) {
    engine.makeInvariant<GlobalCardinalityConst<true>>(violationVarId(), inputs,
                                                       _cover, _low, _up);
  } else {
    engine.makeInvariant<GlobalCardinalityConst<true>>(_intermediate, inputs,
                                                       _cover, _low, _up);
  }
}
