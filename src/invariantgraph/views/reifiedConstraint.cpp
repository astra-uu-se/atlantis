#include "invariantgraph/views/reifiedConstraint.hpp"

#include "views/violation2BoolView.hpp"

invariantgraph::ReifiedConstraint::ReifiedConstraint(
    std::unique_ptr<SoftConstraintNode> constraint,
    invariantgraph::VariableNode* r)
    : VariableDefiningNode({r}), _constraint(std::move(constraint)), _r(r) {
  for (const auto& input : _constraint->_inputs) {
    markAsInput(input);

    auto it = std::find(input->_inputFor.begin(), input->_inputFor.end(), _constraint.get());
    assert(it != input->_inputFor.end());
    input->_inputFor.erase(it);
  }

  _constraint->_inputs.clear();
}

void invariantgraph::ReifiedConstraint::registerWithEngine(Engine& engine, VariableDefiningNode::VariableMap& map) {
  _constraint->registerWithEngine(engine, map);

  auto rId =
      engine.makeIntView<Violation2BoolView>(map.at(_constraint->violation()));
  auto variable = definedVariables().at(0);
  map.emplace(variable, rId);
}
