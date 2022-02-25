#include "invariantgraph/invariants/linearNode.hpp"

#include <algorithm>

#include "invariants/linear.hpp"

std::unique_ptr<invariantgraph::LinearNode>
invariantgraph::LinearNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint>& constraint,
    const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
        variableMap) {
  assert(constraint->arguments().size() == 3);
  assert(constraint->annotations().has<fznparser::DefinesVarAnnotation>());

  auto coeffs = std::get<std::vector<std::shared_ptr<fznparser::Literal>>>(
      constraint->arguments()[0]);
  auto vars = std::get<std::vector<std::shared_ptr<fznparser::Literal>>>(
      constraint->arguments()[1]);
  auto sum =
      std::get<std::shared_ptr<fznparser::Literal>>(constraint->arguments()[2]);

  assert(vars.size() == coeffs.size());

  Int sumValue =
      std::dynamic_pointer_cast<fznparser::ValueLiteral>(sum)->value();

  auto definedVar = constraint->annotations()
                        .get<fznparser::DefinesVarAnnotation>()
                        ->defines()
                        .lock();
  auto definedVarPos =
      std::find_if(vars.begin(), vars.end(), [&definedVar](auto literal) {
        auto named =
            std::dynamic_pointer_cast<fznparser::NamedLiteral>(literal);
        return named->name() == definedVar->name();
      });

  assert(definedVarPos != vars.end());
  size_t definedVarIndex = definedVarPos - vars.begin();

  std::vector<invariantgraph::VariableNode*> convertedVars;
  std::vector<Int> convertedCoeffs;

  for (size_t i = 0; i < vars.size(); ++i) {
    assert(coeffs[i]->type() == fznparser::LiteralType::VALUE);
    auto coeff = std::dynamic_pointer_cast<fznparser::ValueLiteral>(coeffs[i]);

    if (i == definedVarIndex) {
      if (coeff->value() != 1 && coeff->value() != -1)
        throw std::runtime_error(
            "Cannot define variable with coefficient which is not +/-1");

      continue;
    }

    assert(vars[i]->type() == fznparser::LiteralType::SEARCH_VARIABLE);
    auto variable = std::dynamic_pointer_cast<fznparser::Variable>(vars[i]);
    convertedVars.emplace_back(variableMap(variable));

    convertedCoeffs.emplace_back(coeff->value());
  }

  auto output = variableMap(definedVar);
  auto linearInv = std::make_unique<invariantgraph::LinearNode>(
      convertedCoeffs, convertedVars, output);
  output->definedByInvariant(linearInv.get());
  output->setOffset(-sumValue);

  return linearInv;
}

void invariantgraph::LinearNode::registerWithEngine(
    Engine& engine, std::function<VarId(VariableNode*)> variableMapper) const {
  std::vector<VarId> variables;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(variables), variableMapper);

  engine.makeInvariant<::Linear>(_coeffs, variables, variableMapper(output()));
}
