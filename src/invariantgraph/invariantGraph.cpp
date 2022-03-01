#include "invariantgraph/invariantGraph.hpp"

#include <numeric>

#include "invariants/linear.hpp"

void invariantgraph::InvariantGraph::apply(Engine& engine) {
  engine.open();
  for (const auto& variable : _variables) applyVariable(engine, variable.get());
  for (const auto& invariant : _invariants)
    applyInvariant(engine, invariant.get());
  for (const auto& constraint : _softConstraints)
    applyConstraint(engine, constraint.get());

  VarId totalViolations =
      engine.makeIntVar(0, 0, totalViolationsUpperBound(engine));
  engine.makeInvariant<Linear>(_violationVars, totalViolations);
  engine.close();
}

void invariantgraph::InvariantGraph::applyVariable(Engine& engine,
                                                   VariableNode* node) {
  if (wasVisited(node)) return;

  node->registerWithEngine(engine, _engineVariables);
}

void invariantgraph::InvariantGraph::applyInvariant(Engine& engine,
                                                    InvariantNode* node) {
  if (wasVisited(node)) return;
  _appliedInvariants.emplace(node);

  node->registerWithEngine(engine, [this, &engine](auto var) {
    applyVariable(engine, var);
    assert(_engineVariables.count(var) > 0);

    return _engineVariables.at(var);
  });
}

void invariantgraph::InvariantGraph::applyConstraint(Engine& engine,
                                                     SoftConstraintNode* node) {
  if (wasVisited(node)) return;
  _appliedSoftConstraints.emplace(node);

  VarId violationVar =
      node->registerWithEngine(engine, [this, &engine](auto var) {
        applyVariable(engine, var);
        assert(_engineVariables.count(var) > 0);

        return _engineVariables.at(var);
      });

  _violationVars.emplace_back(violationVar);
}

Int invariantgraph::InvariantGraph::totalViolationsUpperBound(
    Engine& engine) const {
  return std::accumulate(_violationVars.begin(), _violationVars.end(), 0,
                         [&](auto sum, const auto& var) {
                           return sum + engine.getUpperBound(var);
                         });
}
