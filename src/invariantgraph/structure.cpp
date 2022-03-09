#include "invariantgraph/structure.hpp"

void invariantgraph::ViewNode::registerWithEngine(
    Engine& engine, std::map<VariableNode*, VarId>& map) {
  auto newId = createView(engine, map.at(_input))->getId();
  map.emplace(this, newId);
}

void invariantgraph::VariableNode::registerWithEngine(
    Engine& engine, std::map<VariableNode*, VarId>& map) {
  auto& [lb, ub] = _domain;

  VarId engineVariable = engine.makeIntVar(lb, lb, ub);
  map.emplace(this, engineVariable);
}
