#include "invariantgraph/constraints/globalCardinalityClosedNode.hpp"

#include "../parseHelper.hpp"

static std::vector<invariantgraph::VariableNode*> merge(
    const std::vector<invariantgraph::VariableNode*>& fst,
    const std::vector<invariantgraph::VariableNode*>& snd) {
  std::vector<invariantgraph::VariableNode*> v(fst);
  v.reserve(fst.size() + snd.size());
  v.insert(v.end(), snd.begin(), snd.end());
  return v;
}

invariantgraph::GlobalCardinalityClosedNode::GlobalCardinalityClosedNode(
    std::vector<VariableNode*> x, std::vector<Int> cover,
    std::vector<VariableNode*> counts, VariableNode* r)
    : SoftConstraintNode({}, merge(x, counts), r),
      _inputs(x),
      _cover(cover),
      _counts(counts) {}

invariantgraph::GlobalCardinalityClosedNode::GlobalCardinalityClosedNode(
    std::vector<VariableNode*> x, std::vector<Int> cover,
    std::vector<VariableNode*> counts, bool shouldHold)
    : SoftConstraintNode(shouldHold ? counts : std::vector<VariableNode*>{},
                         shouldHold ? x : merge(x, counts), shouldHold),
      _inputs(x),
      _cover(cover),
      _counts(counts) {}

std::unique_ptr<invariantgraph::GlobalCardinalityClosedNode>
invariantgraph::GlobalCardinalityClosedNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto inputs =
      mappedVariableVector(model, constraint.arguments[0], variableMap);

  auto cover = integerVector(model, constraint.arguments[1]);

  auto counts =
      mappedVariableVector(model, constraint.arguments[2], variableMap);

  if (cover.size() != counts.size()) {
    throw BadArgumentError(
        "GlobalCardinalityClosedNode: cover and counts must have the same "
        "size.");
  }

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
    return std::make_unique<GlobalCardinalityClosedNode>(inputs, cover, counts,
                                                         r);
  }
  assert(r == nullptr);
  return std::make_unique<GlobalCardinalityClosedNode>(inputs, cover, counts,
                                                       shouldHold);
}

bool invariantgraph::GlobalCardinalityClosedNode::prune() {
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
    if (isCoverFixed[j]) {
      continue;
    }
    Int minCover = 0;
    for (auto* var : inCover[j]) {
      assert(var->domain().contains(_cover[j]));
      minCover += static_cast<Int>(var->domain().isConstant());
    }
    definedVariables().at(j)->domain().removeBelow(minCover);
    definedVariables().at(j)->domain().removeAbove(inCover[j].size());
    if (definedVariables().at(j)->domain().upperBound() == 0) {
      for (auto* var : inCover[j]) {
        didChange = true;
        var->domain().removeValue(_cover[j]);
      }
    } else if (definedVariables().at(j)->domain().isConstant() &&
               static_cast<Int>(inCover[j].size()) ==
                   definedVariables().at(j)->domain().upperBound() &&
               minCover < definedVariables().at(j)->domain().upperBound()) {
      // The cover variable is fixed, and exactly maxCover input variables can
      // take the value _cover[j]:
      didChange = true;
      for (auto* var : inCover[j]) {
        var->domain().fix(_cover[j]);
      }
      for (size_t k = 0; k < _cover.size(); ++k) {
        if (j != k && !isCoverFixed[k]) {
          for (auto* const var : inCover[j]) {
            inCover[k].erase(var);
          }
        }
      }
      if (j > 0) {
        // the pruning might cause additional propagation for indices
        // 0..j-1, start the loop again:
        j = 0;
      }
    }
  }
  return didChange;
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
            engine.makeIntView<NotEqualConst>(_shouldFailViol, 0));
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

    engine.makeInvariant<GlobalCardinalityClosed>(violationVarId(),
                                                  countOutputs, inputs, _cover);
  } else {
    assert(_intermediate.size() == _counts.size());
    assert(_violations.size() == _counts.size() + 1);
    if (isReified()) {
      engine.makeInvariant<GlobalCardinalityClosed>(
          _violations.back(), _intermediate, inputs, _cover);
    } else {
      assert(!shouldHold());
      engine.makeInvariant<GlobalCardinalityClosed>(
          _shouldFailViol, _intermediate, inputs, _cover);
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
      engine.makeInvariant<Linear>(violationVarId(), _violations);
    } else {
      // To hold, only one count must not be equal to its corresponding
      // intermediate:
      engine.makeInvariant<Exists>(violationVarId(), _violations);
    }
  }
}
