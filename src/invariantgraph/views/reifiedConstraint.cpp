#include "invariantgraph/views/reifiedConstraint.hpp"

#include "views/violation2BoolView.hpp"

invariantgraph::ReifiedConstraint::ReifiedConstraint(
    std::unique_ptr<SoftConstraintNode> constraint,
    invariantgraph::VariableNode* r)
    : VariableDefiningNode({r}), _constraint(std::move(constraint)), _r(r) {
  for (const auto& input : _constraint->_inputs) {
    markAsInput(input);

    auto it = std::find(input->_inputFor.begin(), input->_inputFor.end(),
                        _constraint.get());
    assert(it != input->_inputFor.end());
    input->_inputFor.erase(it);
  }

  _constraint->_inputs.clear();
}

void invariantgraph::ReifiedConstraint::createDefinedVariables(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  _constraint->createDefinedVariables(engine, variableMap);

  if (!variableMap.contains(definedVariables().at(0))) {
    const auto rId = engine.makeIntView<Violation2BoolView>(
        variableMap.at(_constraint->violation()));
    variableMap.emplace(definedVariables().at(0), rId);
  }
}

void invariantgraph::ReifiedConstraint::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  _constraint->registerWithEngine(engine, variableMap);
}
