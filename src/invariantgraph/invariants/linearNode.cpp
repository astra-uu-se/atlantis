#include "invariantgraph/invariants/linearNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "invariants/linear.hpp"
#include "views/intOffsetView.hpp"

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
  auto output = *varsIt;
  vars.erase(varsIt);

  auto linearInv =
      std::make_unique<invariantgraph::LinearNode>(coeffs, vars, output, -sum);

  return linearInv;
}

void invariantgraph::LinearNode::registerWithEngine(
    Engine& engine, std::map<VariableNode*, VarId>& variableMap) {
  std::vector<VarId> variables;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(variables),
                 [&](const auto& node) { return variableMap.at(node); });

  VarId intermediate = variables[0];

  // If there is only one variable in the input, there is no need to use a
  // linear invariant.
  if (variables.size() > 1 && _coeffs[0] == 1) {
    const auto& [lb, ub] = getIntermediateDomain();
    intermediate = engine.makeIntVar(lb, lb, ub);
    engine.makeInvariant<::Linear>(_coeffs, variables, intermediate);
  }

  auto outputVar = engine.makeIntView<IntOffsetView>(intermediate, _offset);
  variableMap.emplace(definedVariables()[0], outputVar);
}

std::pair<Int, Int> invariantgraph::LinearNode::getIntermediateDomain() const {
  Int lb = 0, ub = 0;

  for (size_t i = 0; i < _coeffs.size(); i++) {
    const auto& [varLb, varUb] = _variables[i]->domain();

    lb += _coeffs[i] * varLb;
    ub += _coeffs[i] * varUb;
  }

  return {lb, ub};
}
