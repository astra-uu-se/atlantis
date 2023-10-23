#include "invariantgraph/violationInvariantNodes/globalCardinalityClosedNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

GlobalCardinalityClosedNode::GlobalCardinalityClosedNode(
    std::vector<VarNodeId>&& x, std::vector<Int>&& cover,
    std::vector<VarNodeId>&& counts, VarNodeId r)
    : ViolationInvariantNode({}, concat(x, counts), r),
      _inputs(std::move(x)),
      _cover(std::move(cover)),
      _counts(std::move(counts)) {}

GlobalCardinalityClosedNode::GlobalCardinalityClosedNode(
    std::vector<VarNodeId>&& x, std::vector<Int>&& cover,
    std::vector<VarNodeId>&& counts, bool shouldHold)
    : ViolationInvariantNode(
          shouldHold ? std::vector<VarNodeId>(counts)
                     : std::vector<VarNodeId>{},
          shouldHold ? std::move(std::vector<VarNodeId>{x}) : concat(x, counts),
          shouldHold),
      _inputs(std::move(x)),
      _cover(std::move(cover)),
      _counts(std::move(counts)) {}

std::unique_ptr<GlobalCardinalityClosedNode>
GlobalCardinalityClosedNode::fromModelConstraint(
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
  VarNodeId r = NULL_NODE_ID;

  if (constraint.arguments().size() == 4) {
    const fznparser::BoolArg reified =
        std::get<fznparser::BoolArg>(constraint.arguments().at(3));
    if (reified.isFixed()) {
      shouldHold = reified.toParameter();
    } else {
      r = invariantGraph.createVarNode(reified.var());
    }
  }

  if (r != NULL_NODE_ID) {
    return std::make_unique<GlobalCardinalityClosedNode>(
        std::move(inputs), std::move(cover), std::move(counts), r);
  }
  assert(r == NULL_NODE_ID);
  return std::make_unique<GlobalCardinalityClosedNode>(
      std::move(inputs), std::move(cover), std::move(counts), shouldHold);
}

void GlobalCardinalityClosedNode::registerOutputVariables(
    InvariantGraph& invariantGraph, propagation::Engine& engine) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    registerViolation(invariantGraph, engine);
    if (!isReified() && shouldHold()) {
      for (const auto& countOutput : outputVarNodeIds()) {
        if (invariantGraph.varId(countOutput) == propagation::NULL_ID) {
          invariantGraph.varNode(countOutput)
              .setVarId(engine.makeIntVar(0, 0, _inputs.size()));
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
            engine.makeIntView<propagation::NotEqualConst>(engine, _shouldFailViol, 0));
      } else {
        _violations.emplace_back(engine.makeIntVar(0, 0, _inputs.size()));
      }
    }
  }
}

void GlobalCardinalityClosedNode::registerNode(InvariantGraph& invariantGraph,
                                               propagation::Engine& engine) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  std::vector<propagation::VarId> inputs;
  std::transform(_inputs.begin(), _inputs.end(), std::back_inserter(inputs),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  if (!isReified() && shouldHold()) {
    assert(_intermediate.size() == 0);
    assert(_violations.size() == 0);
    std::vector<propagation::VarId> countOutputs;
    std::transform(_counts.begin(), _counts.end(),
                   std::back_inserter(countOutputs),
                   [&](const auto& id) { return invariantGraph.varId(id); });

    engine.makeInvariant<propagation::GlobalCardinalityClosed>(
        engine, violationVarId(invariantGraph), countOutputs, inputs, _cover);
  } else {
    assert(_intermediate.size() == _counts.size());
    assert(_violations.size() == _counts.size() + 1);
    if (isReified()) {
      engine.makeInvariant<propagation::GlobalCardinalityClosed>(
          engine, _violations.back(), _intermediate, inputs, _cover);
    } else {
      assert(!shouldHold());
      engine.makeInvariant<propagation::GlobalCardinalityClosed>(
          engine, _shouldFailViol, _intermediate, inputs, _cover);
    }
    for (size_t i = 0; i < _counts.size(); ++i) {
      if (shouldHold()) {
        engine.makeConstraint<propagation::Equal>(engine, _violations.at(i),
                                     _intermediate.at(i),
                                     invariantGraph.varId(_counts.at(i)));
      } else {
        engine.makeConstraint<propagation::NotEqual>(engine, _violations.at(i),
                                        _intermediate.at(i),
                                        invariantGraph.varId(_counts.at(i)));
      }
    }
    if (shouldHold()) {
      // To hold, each count must be equal to its corresponding intermediate:
      engine.makeInvariant<propagation::Linear>(engine, violationVarId(invariantGraph),
                                   _violations);
    } else {
      // To hold, only one count must not be equal to its corresponding
      // intermediate:
      engine.makeInvariant<propagation::Exists>(engine, violationVarId(invariantGraph),
                                   _violations);
    }
  }
}

}  // namespace invariantgraph