#include "invariantgraph/invariants/max.hpp"

#include <algorithm>

#include "invariants/maxSparse.hpp"

std::shared_ptr<invariantgraph::MaxInvariantNode>
invariantgraph::MaxInvariantNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint>& constraint,
    const std::function<std::shared_ptr<VariableNode>(
        std::shared_ptr<fznparser::Variable>)>& variableMap) {
  assert(constraint->name() == "int_max" ||
         constraint->name() == "array_int_maximum");
  assert(constraint->annotations().has<fznparser::DefinesVarAnnotation>());

  auto definedVar =
      constraint->annotations().get<fznparser::DefinesVarAnnotation>()->defines().lock();

  if (constraint->name() == "int_max") {
    assert(constraint->arguments().size() == 3);

    auto a = std::get<std::shared_ptr<fznparser::Literal>>(
        constraint->arguments()[0]);
    auto b = std::get<std::shared_ptr<fznparser::Literal>>(
        constraint->arguments()[1]);
    auto c = std::get<std::shared_ptr<fznparser::Literal>>(
        constraint->arguments()[2]);

    assert(definedVar == c);

    auto aNode = variableMap(std::dynamic_pointer_cast<fznparser::Variable>(a));
    auto bNode = variableMap(std::dynamic_pointer_cast<fznparser::Variable>(b));
    auto cNode = variableMap(std::dynamic_pointer_cast<fznparser::Variable>(c));

    return std::make_shared<invariantgraph::MaxInvariantNode>(
        std::vector<std::shared_ptr<VariableNode>>{aNode, bNode}, cNode);
  } else if (constraint->name() == "array_int_maximum") {
    assert(constraint->arguments().size() == 2);

    auto inputs = std::get<std::vector<std::shared_ptr<fznparser::Literal>>>(
        constraint->arguments()[1]);

    std::vector<std::shared_ptr<VariableNode>> inputNodes;
    std::transform(inputs.begin(), inputs.end(), std::back_inserter(inputNodes),
                   [&variableMap](const auto& var) {
                     return variableMap(
                         std::dynamic_pointer_cast<fznparser::Variable>(var));
                   });

    auto output = std::get<std::shared_ptr<fznparser::Literal>>(
        constraint->arguments()[0]);
    assert(definedVar == output);

    auto outputNode =
        variableMap(std::dynamic_pointer_cast<fznparser::Variable>(output));

    return std::make_shared<invariantgraph::MaxInvariantNode>(inputNodes,
                                                              outputNode);
  }

  throw std::runtime_error(
      std::string("Cannot create Max invariant from constraint ")
          .append(constraint->name()));
}

void invariantgraph::MaxInvariantNode::registerWithEngine(
    Engine& engine,
    std::function<VarId(const std::shared_ptr<VariableNode>&)> variableMapper)
    const {
  std::vector<VarId> variables;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(variables), variableMapper);

  engine.makeInvariant<::MaxSparse>(variables, variableMapper(output()));
}
