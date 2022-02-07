#include "invariantgraph/invariantGraph.hpp"

#include <algorithm>
#include <numeric>

#include "invariants/linear.hpp"

void invariantgraph::InvariantGraph::apply(Engine& engine) {
  for (const auto& violationVariable : violationVars)
    applyVariable(engine, violationVariable);

  VarId totalViolations =
      engine.makeIntVar(0, 0, totalViolationsUpperBound(engine));
  engine.makeInvariant<Linear>(getViolationVariables(), totalViolations);
}

void invariantgraph::InvariantGraph::applyVariable(
    Engine& engine, const std::shared_ptr<VariableNode>& node) {
  if (wasVisited(node)) return;

  // TODO: Different types of domains.
  assert(node->variable()->domain()->type() == fznparser::DomainType::INT);
  fznparser::IntDomain* domain =
      dynamic_cast<fznparser::IntDomain*>(node->variable()->domain());

  // TODO: Initial assignment.
  VarId engineVariable = engine.makeIntVar(
      domain->lowerBound(), domain->lowerBound(), domain->upperBound());
  engineVariables.emplace(node, engineVariable);

  if (node->isFunctionallyDefined()) {
    std::shared_ptr<InvariantNode> definedBy =
        node->definingInvariant().value();
    applyInvariant(engine, definedBy);
  }

  for (const auto& constraint : node->softConstraints()) {
    applyConstraint(engine, constraint);
  }
}

void invariantgraph::InvariantGraph::applyInvariant(
    Engine& engine, const std::shared_ptr<InvariantNode>& node) {
  if (wasVisited(node)) return;
  appliedInvariants.emplace(node);

  node->registerWithEngine(engine, [this, &engine](auto var) {
    applyVariable(engine, var);
    return engineVariables.at(var);
  });
}

void invariantgraph::InvariantGraph::applyConstraint(
    Engine& engine, const std::shared_ptr<SoftConstraintNode>& node) {
  if (wasVisited(node)) return;
  appliedSoftConstraints.emplace(node);

  VarId violationVar =
      node->registerWithEngine(engine, [this, &engine](auto var) {
        applyVariable(engine, var);
        return engineVariables.at(var);
      });

  engineVariables.emplace(node->violationNode(), violationVar);
}

Int invariantgraph::InvariantGraph::totalViolationsUpperBound(
    Engine& engine) const {
  return std::transform_reduce(
      violationVars.begin(), violationVars.end(), 0,
      [this, &engine](auto node) {
        return engine.getUpperBound(engineVariables.at(node));
      },
      std::plus<>());
}

std::vector<VarId> invariantgraph::InvariantGraph::getViolationVariables()
    const {
  std::vector<VarId> variables;
  variables.reserve(violationVars.size());

  std::transform(violationVars.begin(), violationVars.end(),
                 std::back_inserter(variables),
                 [this](auto node) { return engineVariables.at(node); });

  return variables;
}
