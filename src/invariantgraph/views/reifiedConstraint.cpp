#include "invariantgraph/views/reifiedConstraint.hpp"
#include "views/boolView.hpp"

void invariantgraph::ReifiedConstraint::registerWithEngine(
    Engine& engine, std::map<VariableNode*, VarId>& map) {
  VarId violationVar = _constraint->registerWithEngine(
      engine, [&](const auto& node) { return map.at(node); });

  auto rId = createView(engine, violationVar)->getId();
  map.emplace(_r, rId);
}

std::shared_ptr<View> invariantgraph::ReifiedConstraint::createView(
    Engine& engine, VarId variable) const {
  return engine.makeIntView<BoolView>(variable);
}
