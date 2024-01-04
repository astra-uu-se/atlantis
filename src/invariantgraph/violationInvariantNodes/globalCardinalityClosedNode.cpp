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

void GlobalCardinalityClosedNode::registerNode(
    InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
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
        solver.makeViolationInvariant<propagation::Equal>(
            solver, _violations.at(i), _intermediate.at(i),
            invariantGraph.varId(_counts.at(i)));
      } else {
        solver.makeViolationInvariant<propagation::NotEqual>(
            solver, _violations.at(i), _intermediate.at(i),
            invariantGraph.varId(_counts.at(i)));
      }
    }
    if (shouldHold()) {
      // To hold, each count must be equal to its corresponding intermediate:
      solver.makeInvariant<propagation::Linear>(
          solver, violationVarId(invariantGraph), _violations);
    } else {
      // To hold, only one count must not be equal to its corresponding
      // intermediate:
      solver.makeInvariant<propagation::Exists>(
          solver, violationVarId(invariantGraph), _violations);
    }
  }
}

}  // namespace atlantis::invariantgraph