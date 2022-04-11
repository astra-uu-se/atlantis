#include "invariantgraph/views/reifiedConstraint.hpp"

#include "views/violation2BoolView.hpp"

void invariantgraph::ReifiedConstraint::registerWithEngine(Engine& engine, VariableDefiningNode::VariableMap& map) {
  _constraint->registerWithEngine(engine, map);

  auto rId =
      engine.makeIntView<Violation2BoolView>(map.at(_constraint->violation()));
  auto variable = definedVariables().at(0);
  map.emplace(variable, rId);
}
