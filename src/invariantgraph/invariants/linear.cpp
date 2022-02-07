#include "invariantgraph/invariants/linear.hpp"

#include <algorithm>

#include "invariants/linear.hpp"

void invariantgraph::LinearInvariantNode::registerWithEngine(
    Engine& engine,
    std::function<VarId(const std::shared_ptr<VariableNode>&)> variableMapper)
    const {
  std::vector<VarId> variables;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(variables), variableMapper);

  engine.makeInvariant<::Linear>(_coeffs, variables, variableMapper(_output));
}
