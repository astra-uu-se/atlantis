#include "invariantgraph/invariantGraph.hpp"

#include <limits>
#include <numeric>

#include "invariants/linear.hpp"

std::map<VarId, std::shared_ptr<fznparser::Variable>>
invariantgraph::InvariantGraph::apply(Engine& engine) {
  engine.open();
  for (const auto& variable : _variables) applyVariable(engine, variable.get());
  for (const auto& invariant : _invariants)
    applyInvariant(engine, invariant.get());

  violationVars.reserve(_softConstraints.size());
  for (const auto& softConstraint : _softConstraints)
    applyConstraint(engine, softConstraint.get());

  Int ub = totalViolationsUpperBound(engine);

  // The upper bound overflowed the Int type.
  if (ub < 0) ub = std::numeric_limits<Int>::max();

  _totalViolations.emplace(engine.makeIntVar(0, 0, ub));
  engine.makeInvariant<Linear>(violationVars, *_totalViolations);
  engine.close();

  std::map<VarId, std::shared_ptr<fznparser::Variable>> variableMap;
  for (auto& [variableNode, varId] : engineVariables)
    variableMap.emplace(varId, variableNode->variable());

  return variableMap;
}

void invariantgraph::InvariantGraph::applyVariable(Engine& engine,
                                                   VariableNode* node) {
  // TODO: Different types of domains.
  assert(node->variable()->domain()->type() == fznparser::DomainType::INT);
  fznparser::IntDomain* domain =
      dynamic_cast<fznparser::IntDomain*>(node->variable()->domain());

  VarId engineVariable = engine.makeIntVar(
      domain->lowerBound(), domain->lowerBound(), domain->upperBound());
  engineVariables.emplace(node, engineVariable);
}

void invariantgraph::InvariantGraph::applyInvariant(Engine& engine,
                                                    InvariantNode* node) {
  node->registerWithEngine(engine,
                           [&](auto var) { return engineVariables.at(var); });
}

void invariantgraph::InvariantGraph::applyConstraint(Engine& engine,
                                                     SoftConstraintNode* node) {
  VarId violationVar = node->registerWithEngine(
      engine, [this](auto var) { return engineVariables.at(var); });

  violationVars.push_back(violationVar);
}

Int invariantgraph::InvariantGraph::totalViolationsUpperBound(
    Engine& engine) const {
  return std::transform_reduce(
      violationVars.begin(), violationVars.end(), 0, std::plus<>(),
      [&engine](auto var) { return engine.getUpperBound(var); });
}
