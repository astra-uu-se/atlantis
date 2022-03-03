#include "invariantgraph/structure.hpp"

void invariantgraph::ViewNode::registerWithEngine(
    Engine& engine, std::map<VariableNode*, VarId>& map) {
  auto newId = createView(engine, map.at(_input))->getId();
  map.emplace(this, newId);
}

void invariantgraph::VariableNode::registerWithEngine(
    Engine& engine, std::map<VariableNode*, VarId>& map) {
  // TODO: Different types of domains.
  assert(variable()->domain()->type() == fznparser::DomainType::INT);
  fznparser::IntDomain* domain =
      dynamic_cast<fznparser::IntDomain*>(variable()->domain());

  // TODO: Initial assignment.
  VarId engineVariable = engine.makeIntVar(
      domain->lowerBound(), domain->lowerBound(), domain->upperBound());
  map.emplace(this, engineVariable);
}

invariantgraph::SoftConstraintNode::SoftConstraintNode(
    const std::vector<VariableNode*>& variables) {
  for (const auto& variable : variables)
    variable->addSoftConstraint(this);
}
