#include "invariantgraph/constraints/globalCardinalityClosedNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<GlobalCardinalityClosedNode>
GlobalCardinalityClosedNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  std::vector<VariableNode*> inputs = invariantGraph.addVariableArray(
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0)));

  std::vector<Int> cover =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(1))
          .toParVector();

  std::vector<VariableNode*> counts = invariantGraph.addVariableArray(
      std::get<fznparser::IntVarArray>(constraint.arguments().at(2)));

  assert(cover.size() == counts.size());

  bool shouldHold = true;
  VariableNode* r = nullptr;

  if (constraint.arguments().size() == 4) {
    const fznparser::BoolArg reified =
        std::get<fznparser::BoolArg>(constraint.arguments().at(3));
    if (std::holds_alternative<bool>(reified)) {
      shouldHold = std::get<bool>(reified);
    } else {
      r = invariantGraph.addVariable(
          std::get<std::reference_wrapper<const fznparser::BoolVar>>(reified)
              .get());
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

void GlobalCardinalityClosedNode::createDefinedVariables(Engine& engine) {
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
            engine.makeIntView<NotEqualConst>(engine, _shouldFailViol, 0));
      } else {
        _violations.emplace_back(engine.makeIntVar(0, 0, _inputs.size()));
      }
    }
  }
}

void GlobalCardinalityClosedNode::registerWithEngine(Engine& engine) {
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

    engine.makeInvariant<GlobalCardinalityClosed>(engine, violationVarId(),
                                                  countOutputs, inputs, _cover);
  } else {
    assert(_intermediate.size() == _counts.size());
    assert(_violations.size() == _counts.size() + 1);
    if (isReified()) {
      engine.makeInvariant<GlobalCardinalityClosed>(
          engine, _violations.back(), _intermediate, inputs, _cover);
    } else {
      assert(!shouldHold());
      engine.makeInvariant<GlobalCardinalityClosed>(
          engine, _shouldFailViol, _intermediate, inputs, _cover);
    }
    for (size_t i = 0; i < _counts.size(); ++i) {
      if (shouldHold()) {
        engine.makeConstraint<Equal>(engine, _violations.at(i),
                                     _intermediate.at(i),
                                     _counts.at(i)->varId());
      } else {
        engine.makeConstraint<NotEqual>(engine, _violations.at(i),
                                        _intermediate.at(i),
                                        _counts.at(i)->varId());
      }
    }
    if (shouldHold()) {
      // To hold, each count must be equal to its corresponding intermediate:
      engine.makeInvariant<Linear>(engine, violationVarId(), _violations);
    } else {
      // To hold, only one count must not be equal to its corresponding
      // intermediate:
      engine.makeInvariant<Exists>(engine, violationVarId(), _violations);
    }
  }
}

}  // namespace invariantgraph