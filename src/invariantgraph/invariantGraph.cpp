#include "invariantgraph/invariantGraph.hpp"

#include <numeric>

#include "invariants/linear.hpp"

void invariantgraph::InvariantGraph::apply(Engine& engine) {
  engine.open();
  for (const auto& variable : _variables) applyVariable(engine, variable.get());

  VarId totalViolations =
      engine.makeIntVar(0, 0, totalViolationsUpperBound(engine));
  engine.makeInvariant<Linear>(_violationVars, totalViolations);
  engine.close();
}

void invariantgraph::InvariantGraph::applyVariable(Engine& engine,
                                                   VariableNode* node) {
  if (wasVisited(node)) return;

  // TODO: Different types of domains.
  assert(node->variable()->domain()->type() == fznparser::DomainType::INT);
  fznparser::IntDomain* domain =
      dynamic_cast<fznparser::IntDomain*>(node->variable()->domain());

  // TODO: Initial assignment.
  VarId engineVariable = engine.makeIntVar(
      domain->lowerBound(), domain->lowerBound(), domain->upperBound());
  _engineVariables.emplace(node, engineVariable);

  if (node->isFunctionallyDefined()) {
    auto definedBy = *node->definingInvariant();
    applyInvariant(engine, definedBy);
  }

  for (const auto& constraint : node->softConstraints()) {
    applyConstraint(engine, constraint);
  }
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
  return std::transform_reduce(
      _violationVars.begin(), _violationVars.end(), 0, std::plus<>(),
      [&engine](auto var) { return engine.upperBound(var); });
}
