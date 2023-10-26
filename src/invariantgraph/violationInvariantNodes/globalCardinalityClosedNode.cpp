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
    const fznparser::Constraint& constraint, FznInvariantGraph& invariantGraph) {
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

void GlobalCardinalityClosedNode::registerOutputVars(
    InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    registerViolation(invariantGraph, solver);
    if (!isReified() && shouldHold()) {
      for (const auto& countOutput : outputVarNodeIds()) {
        if (invariantGraph.varId(countOutput) == propagation::NULL_ID) {
          invariantGraph.varNode(countOutput)
              .setVarId(solver.makeIntVar(0, 0, _inputs.size()));
        }
      }
    } else {
      for (size_t i = 0; i < _counts.size(); ++i) {
        _intermediate.emplace_back(solver.makeIntVar(0, 0, _inputs.size()));
      }
      for (size_t i = 0; i < _counts.size(); ++i) {
        _violations.emplace_back(solver.makeIntVar(0, 0, _inputs.size()));
      }
      // intermediate viol for GCC closed constraint
      if (!shouldHold()) {
        _shouldFailViol = solver.makeIntVar(0, 0, _inputs.size());
        _violations.emplace_back(
            solver.makeIntView<propagation::NotEqualConst>(solver, _shouldFailViol, 0));
      } else {
        _violations.emplace_back(solver.makeIntVar(0, 0, _inputs.size()));
      }
    }
  }
}

void GlobalCardinalityClosedNode::registerNode(InvariantGraph& invariantGraph,
                                               propagation::SolverBase& solver) {
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

    solver.makeInvariant<propagation::GlobalCardinalityClosed>(
        solver, violationVarId(invariantGraph), countOutputs, inputs, _cover);
  } else {
    assert(_intermediate.size() == _counts.size());
    assert(_violations.size() == _counts.size() + 1);
    if (isReified()) {
      solver.makeInvariant<propagation::GlobalCardinalityClosed>(
          solver, _violations.back(), _intermediate, inputs, _cover);
    } else {
      assert(!shouldHold());
      solver.makeInvariant<propagation::GlobalCardinalityClosed>(
          solver, _shouldFailViol, _intermediate, inputs, _cover);
    }
    for (size_t i = 0; i < _counts.size(); ++i) {
      if (shouldHold()) {
        solver.makeViolationInvariant<propagation::Equal>(solver, _violations.at(i),
                                     _intermediate.at(i),
                                     invariantGraph.varId(_counts.at(i)));
      } else {
        solver.makeViolationInvariant<propagation::NotEqual>(solver, _violations.at(i),
                                        _intermediate.at(i),
                                        invariantGraph.varId(_counts.at(i)));
      }
    }
    if (shouldHold()) {
      // To hold, each count must be equal to its corresponding intermediate:
      solver.makeInvariant<propagation::Linear>(solver, violationVarId(invariantGraph),
                                   _violations);
    } else {
      // To hold, only one count must not be equal to its corresponding
      // intermediate:
      solver.makeInvariant<propagation::Exists>(solver, violationVarId(invariantGraph),
                                   _violations);
    }
  }
}

}  // namespace invariantgraph