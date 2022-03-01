#include "invariantgraph/invariants/linearNode.hpp"

#include <algorithm>

#include "invariantgraph/parseHelper.hpp"
#include "invariants/linear.hpp"

std::unique_ptr<invariantgraph::LinearNode>
invariantgraph::LinearNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint>& constraint,
    const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
        variableMap) {
  assert(constraint->arguments().size() == 3);
  assert(constraint->annotations().has<fznparser::DefinesVarAnnotation>());

  VALUE_VECTOR_ARG(coeffs, constraint->arguments()[0]);
  MAPPED_SEARCH_VARIABLE_VECTOR_ARG(vars, constraint->arguments()[1],
                                    variableMap);
  VALUE_ARG(sum, constraint->arguments()[2]);

  auto definedVar = constraint->annotations()
                        .get<fznparser::DefinesVarAnnotation>()
                        ->defines()
                        .lock();
  auto definedVarPos =
      std::find_if(vars.begin(), vars.end(), [&definedVar](auto varNode) {
        return varNode->variable()->name() == definedVar->name();
      });

  assert(definedVarPos != vars.end());
  size_t definedVarIndex = definedVarPos - vars.begin();

  if (std::abs(coeffs[definedVarIndex]) != 1) {
    throw std::runtime_error(
        "Cannot define variable with coefficient which is not +/-1");
  }

  auto coeffsIt = coeffs.begin();
  std::advance(coeffsIt, definedVarIndex);
  coeffs.erase(coeffsIt);

  auto varsIt = vars.begin();
  std::advance(varsIt, definedVarIndex);
  vars.erase(varsIt);

  auto output = variableMap(definedVar);
  auto linearInv =
      std::make_unique<invariantgraph::LinearNode>(coeffs, vars, output);
  output->definedByInvariant(linearInv.get());
  output->setOffset(-sum);

  return linearInv;
}

void invariantgraph::LinearNode::registerWithEngine(
    Engine& engine, std::function<VarId(VariableNode*)> variableMapper) const {
  std::vector<VarId> variables;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(variables), variableMapper);

  engine.makeInvariant<::Linear>(_coeffs, variables, variableMapper(output()));
}
