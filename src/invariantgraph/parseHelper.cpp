#include "parseHelper.hpp"

#include <algorithm>
#include <optional>
#include <variant>

#include "utils/variant.hpp"

#define EMPTY_VARIANT_BRANCH(ParamType, ReturnType, message) \
  [](const ParamType &) {                                    \
    throw std::runtime_error(message);                       \
    return static_cast<ReturnType>(0);                       \
  }

#define DEFAULT_EMPTY_VARIANT_BRANCH(ReturnType, message) \
  EMPTY_VARIANT_BRANCH(auto, ReturnType, message)

using namespace fznparser;

template <typename T>
static invariantgraph::VariableNode *createNode(
    T element, const std::function<invariantgraph::VariableNode *(
                   invariantgraph::MappableValue &)> &variableFactory) {
  invariantgraph::MappableValue value(element);
  return variableFactory(value);
}

std::vector<invariantgraph::VariableNode *> mappedVariableVector(
    const FZNConstraint::Argument &argument,
    const std::function<invariantgraph::VariableNode *(
        invariantgraph::MappableValue &)> &variableMap) {
  if (!std::holds_alternative<FZNConstraint::ArrayArgument>(argument)) {
    throw std::runtime_error("Expected constraint argument to be an array.");
  }

  auto array = std::get<FZNConstraint::ArrayArgument>(argument);

  std::vector<invariantgraph::VariableNode *> nodes;
  nodes.reserve(array.size());

  std::transform(
      array.begin(), array.end(), std::back_inserter(nodes),
      [&](const auto &element) {
        return std::visit<invariantgraph::VariableNode *>(
            overloaded{EMPTY_VARIANT_BRANCH(
                           Set<Int>, invariantgraph::VariableNode *,
                           "Cannot parse Set<Int> as a variable node."),
                       [&](const Identifier &identifier) {
                         return createNode(identifier, variableMap);
                       },
                       [&](const auto &value) {
                         return createNode(value, variableMap);
                       }},
            element);
      });

  return nodes;
}

invariantgraph::VariableNode *mappedVariable(
    const fznparser::Constraint::Argument &argument,
    const std::function<invariantgraph::VariableNode *(
        invariantgraph::MappableValue &)> &variableMap) {
  return std::visit<invariantgraph::VariableNode *>(
      overloaded{[&](bool b) { return createNode(b, variableMap); },
                 [&](Int i) { return createNode(i, variableMap); },
                 [&](const fznparser::Identifier &ident) {
                   return createNode(ident, variableMap);
                 },
                 DEFAULT_EMPTY_VARIANT_BRANCH(
                     invariantgraph::VariableNode *,
                     "Unexpected variant for variable node ptr.")},
      argument);
}

template <typename T>
static T valueFromParameter(const Parameter &param) {
  return std::get<BaseParameter<T>>(param).value;
}

static std::vector<Int> intLiteralVector(
    const fznparser::FZNModel &model,
    const FZNConstraint::ArrayArgument &array) {
  std::vector<Int> values;
  values.reserve(array.size());

  for (const auto &element : array) {
    auto value = std::visit<Int>(
        overloaded{
            [](Int v) { return v; },
            [&](const Identifier &identifier) {
              auto parameter = std::get<Parameter>(*model.identify(identifier));
              return valueFromParameter<Int>(parameter);
            },
            DEFAULT_EMPTY_VARIANT_BRANCH(Int, "Unexpected variant branch.")},
        element);

    values.push_back(value);
  }

  return values;
}

std::vector<Int> integerVector(const fznparser::FZNModel &model,
                               const FZNConstraint::Argument &argument) {
  return std::visit<std::vector<Int>>(
      overloaded{[&](const FZNConstraint::ArrayArgument &array) {
                   return intLiteralVector(model, array);
                 },
                 [&](const Identifier &identifier) {
                   auto param =
                       std::get<Parameter>(*model.identify(identifier));
                   return valueFromParameter<std::vector<Int>>(param);
                 },
                 DEFAULT_EMPTY_VARIANT_BRANCH(
                     std::vector<Int>,
                     "Unexpected variant for value literal vector element.")},
      argument);
}

Int integerValue(const FZNModel &model,
                 const FZNConstraint::Argument &argument) {
  return std::visit<Int>(
      overloaded{[](Int value) { return value; },
                 [&](const Identifier &identifier) {
                   auto parameter =
                       std::get<Parameter>(*model.identify(identifier));
                   return valueFromParameter<Int>(parameter);
                 },
                 DEFAULT_EMPTY_VARIANT_BRANCH(
                     Int, "Invalid variant for an integer value.")},
      argument);
}

bool definesVariable(const FZNConstraint &constraint,
                     const FZNSearchVariable &variable) {
  auto definedVariableId = definedVariable(constraint);
  if (!definedVariableId) return false;

  return std::visit<bool>(
      [&](const auto &var) { return definedVariableId == var.name; }, variable);
}
