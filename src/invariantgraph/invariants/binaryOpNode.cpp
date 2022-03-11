#include "invariantgraph/invariants/binaryOpNode.hpp"

#include "../parseHelper.hpp"
#include "invariantgraph/invariants/intDivNode.hpp"
#include "invariantgraph/invariants/intModNode.hpp"
#include "invariantgraph/invariants/intPowNode.hpp"
#include "invariantgraph/invariants/intTimesNode.hpp"

template <typename T>
std::unique_ptr<T> invariantgraph::BinaryOpNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint>& constraint,
    const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
        variableMap) {
  static_assert(std::is_base_of<BinaryOpNode, T>{});

  assert(constraint->name() == T::constraint_name());
  assert(constraint->annotations().has<fznparser::DefinesVarAnnotation>());
  assert(constraint->arguments().size() == 3);

  auto definedVar = constraint->annotations()
                        .get<fznparser::DefinesVarAnnotation>()
                        ->defines()
                        .lock();

  MAPPED_SEARCH_VARIABLE_ARG(a, constraint->arguments()[0], variableMap);
  MAPPED_SEARCH_VARIABLE_ARG(b, constraint->arguments()[1], variableMap);
  MAPPED_SEARCH_VARIABLE_ARG(output, constraint->arguments()[2], variableMap);

  assert(definedVar == output->variable());

  return std::make_unique<T>(a, b, output);
}

void invariantgraph::BinaryOpNode::registerWithEngine(
    Engine& engine, std::map<VariableNode*, VarId>& variableMap) {
  auto outputId =
      registerDefinedVariable(engine, variableMap, definedVariables()[0]);
  createInvariant(engine, variableMap.at(_a), variableMap.at(_b), outputId);
}

// Instantiation of the binary operator factory methods.
template std::unique_ptr<invariantgraph::IntDivNode>
invariantgraph::BinaryOpNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint>& constraint,
    const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
        variableMap);

template std::unique_ptr<invariantgraph::IntModNode>
invariantgraph::BinaryOpNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint>& constraint,
    const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
        variableMap);

template std::unique_ptr<invariantgraph::IntTimesNode>
invariantgraph::BinaryOpNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint>& constraint,
    const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
        variableMap);

template std::unique_ptr<invariantgraph::IntPowNode>
invariantgraph::BinaryOpNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint>& constraint,
    const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
        variableMap);
