#include "invariantgraph/views/reifiedConstraint.hpp"

#include "views/violation2Bool.hpp"

void invariantgraph::ReifiedConstraint::registerWithEngine(
    Engine& engine, std::map<VariableNode*, VarId>& map) {
  _constraint->registerWithEngine(engine, map);

  auto rId =
      engine.makeIntView<Violation2Bool>(map.at(_constraint->violation()));
  auto variable = definedVariables().at(0);
  map.emplace(variable, rId);
}
