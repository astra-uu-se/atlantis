#include "invariantgraph/violationInvariantNodes/globalCardinalityNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<GlobalCardinalityNode>
GlobalCardinalityNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  std::vector<VarNodeId> inputs = invariantGraph.createVarNodes(
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0)));

  std::vector<Int> cover =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(1))
          .toParVector();

  std::vector<VarNodeId> counts = invariantGraph.createVarNodes(
      std::get<fznparser::IntVarArray>(constraint.arguments().at(2)));

  assert(cover.size() == counts.size());

  bool shouldHold = true;
  VarNodeId r = nullptr;

  if (constraint.arguments().size() == 4) {
    const fznparser::BoolArg reified =
        std::get<fznparser::BoolArg>(constraint.arguments().at(3));
    if (reified.isFixed()) {
      shouldHold = reified.toParameter();
    } else {
      r = invariantGraph.createVarNode(reified.var());
    }
  }

  if (r != nullptr) {
    return std::make_unique<GlobalCardinalityNode>(
        std::move(inputs), std::move(cover), std::move(counts), r);
  }
  assert(r == nullptr);
  return std::make_unique<GlobalCardinalityNode>(
      std::move(inputs), std::move(cover), std::move(counts), shouldHold);
}

void GlobalCardinalityNode::registerOutputVariables(
    InvariantGraph& invariantGraph, Engine& engine) {
  if (!isReified() && shouldHold()) {
    for (auto* const countOutput : outputVarNodeIds()) {
      if (countOutput->varId() == NULL_ID) {
        countOutput->setVarId(engine.makeIntVar(0, 0, _inputs.size()));
      }
    }
  } else if (violationVarId() == NULL_ID) {
    registerViolation(engine);

    for (size_t i = 0; i < _counts.size(); ++i) {
      _intermediate.emplace_back(engine.makeIntVar(0, 0, _inputs.size()));
    }
    if (_counts.size() == 1) {
      _violations.emplace_back(violationVarId());
    } else {
      for (size_t i = 0; i < _counts.size(); ++i) {
        _violations.emplace_back(engine.makeIntVar(0, 0, _inputs.size()));
      }
    }
  }
}

void GlobalCardinalityNode::registerNode(InvariantGraph& invariantGraph,
                                         Engine& engine) {
  std::vector<VarId> inputs;
  std::transform(_inputs.begin(), _inputs.end(), std::back_inserter(inputs),
                 [&](auto node) { return node->varId(); });

  if (!isReified() && shouldHold()) {
    std::vector<VarId> countOutputs;
    std::transform(_counts.begin(), _counts.end(),
                   std::back_inserter(countOutputs),
                   [&](auto node) { return node->varId(); });

    engine.makeInvariant<GlobalCardinalityOpen>(engine, countOutputs, inputs,
                                                _cover);
  } else {
    assert(violationVarId() != NULL_ID);
    assert(_intermediate.size() == _counts.size());
    assert(_violations.size() == _counts.size());
    engine.makeInvariant<GlobalCardinalityOpen>(engine, _intermediate, inputs,
                                                _cover);
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
    if (_counts.size() > 1) {
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
}

}  // namespace invariantgraph