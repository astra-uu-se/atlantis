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

static std::vector<invariantgraph::VariableNode *> variableNodesFromLiteral(
    const FZNConstraint::ArrayArgument &argument,
    const std::function<invariantgraph::VariableNode *(
        invariantgraph::MappableValue &)> &variableMap) {
  std::vector<invariantgraph::VariableNode *> nodes;
  nodes.reserve(argument.size());

  std::transform(
      argument.begin(), argument.end(), std::back_inserter(nodes),
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

template <typename T>
static std::vector<invariantgraph::VariableNode *>
variableNodesFromVariableArray(
    const fznparser::VariableArray<T> &argument,
    const std::function<invariantgraph::VariableNode *(
        invariantgraph::MappableValue &)> &variableMap) {
  std::vector<invariantgraph::VariableNode *> nodes;

  for (const auto &element : argument) {
    std::visit(
        [&](const auto &elem) {
          nodes.push_back(createNode(elem, variableMap));
        },
        element);
  }

  return nodes;
}

std::vector<invariantgraph::VariableNode *> mappedVariableVector(
    const fznparser::FZNModel &model, const FZNConstraint::Argument &argument,
    const std::function<invariantgraph::VariableNode *(
        invariantgraph::MappableValue &)> &variableMap) {
  if (std::holds_alternative<FZNConstraint::ArrayArgument>(argument)) {
    auto array = std::get<FZNConstraint::ArrayArgument>(argument);
    return variableNodesFromLiteral(array, variableMap);
  } else if (std::holds_alternative<fznparser::Identifier>(argument)) {
    auto identifier = std::get<fznparser::Identifier>(argument);
    auto identifiable = *model.identify(identifier);

    if (std::holds_alternative<fznparser::Variable>(identifiable)) {
      auto variable = std::get<fznparser::Variable>(identifiable);

      if (std::holds_alternative<fznparser::IntVariableArray>(variable)) {
        return variableNodesFromVariableArray<Int>(
            std::get<fznparser::IntVariableArray>(variable), variableMap);
      } else if (std::holds_alternative<fznparser::BoolVariableArray>(
                     variable)) {
        return variableNodesFromVariableArray<bool>(
            std::get<fznparser::BoolVariableArray>(variable), variableMap);
      }
    }
  }

  throw std::runtime_error(
      "Expected constraint argument to be an array or an identifier for an "
      "array.");
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

template <typename T>
static std::vector<T> valueLiteralVector(
    const fznparser::FZNModel &model,
    const FZNConstraint::ArrayArgument &array) {
  std::vector<T> values;
  values.reserve(array.size());

  for (const auto &element : array) {
    auto value = std::visit<T>(
        overloaded{
            [](T v) { return v; },
            [&](const Identifier &identifier) {
              auto parameter = std::get<Parameter>(*model.identify(identifier));
              return valueFromParameter<T>(parameter);
            },
            DEFAULT_EMPTY_VARIANT_BRANCH(T, "Unexpected variant branch.")},
        element);

    values.push_back(value);
  }

  return values;
}

std::vector<Int> integerVector(const fznparser::FZNModel &model,
                               const FZNConstraint::Argument &argument) {
  return std::visit<std::vector<Int>>(
      overloaded{[&](const FZNConstraint::ArrayArgument &array) {
                   return valueLiteralVector<Int>(model, array);
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
  if (!definedVariableId) {
    return false;
  }

  return std::visit<bool>(
      [&](const auto &var) { return definedVariableId == var.name; }, variable);
}

fznparser::Set<Int> integerSet(const FZNModel &model,
                               const FZNConstraint::Argument &argument) {
  if (std::holds_alternative<Set<Int>>(argument)) {
    return std::get<Set<Int>>(argument);
  }

  assert(std::holds_alternative<fznparser::Identifier>(argument));
  auto identifier = std::get<fznparser::Identifier>(argument);
  auto identifiable = *model.identify(identifier);
  assert(std::holds_alternative<Parameter>(identifiable));
  auto parameter = std::get<Parameter>(identifiable);
  assert(std::holds_alternative<SetOfIntParameter>(parameter));
  return std::get<SetOfIntParameter>(parameter).value;
}

std::vector<Int> boolVectorAsIntVector(
    const FZNModel &model, const FZNConstraint::Argument &argument) {
  auto bools = std::visit<std::vector<bool>>(
      overloaded{
          [&](const fznparser::Identifier &identifier) {
            auto identifiable = *model.identify(identifier);
            return valueFromParameter<std::vector<bool>>(
                std::get<Parameter>(identifiable));
          },
          [&](const FZNConstraint::ArrayArgument &array) {
            return valueLiteralVector<bool>(model, array);
          },
          DEFAULT_EMPTY_VARIANT_BRANCH(
              std::vector<bool>, "Unexpected variant type for bool array.")},
      argument);

  std::vector<Int> ints;
  ints.reserve(bools.size());
  std::transform(bools.begin(), bools.end(), std::back_inserter(ints),
                 [](const bool b) { return 1 - static_cast<Int>(b); });

  return ints;
}

bool booleanValue(const FZNModel &model, const FZNConstraint::Argument &argument) {
  return std::visit<bool>(
      overloaded{[](bool value) { return value; },
                 [&](const Identifier &identifier) {
                   auto parameter =
                       std::get<Parameter>(*model.identify(identifier));
                   return valueFromParameter<bool>(parameter);
                 },
                 DEFAULT_EMPTY_VARIANT_BRANCH(
                     Int, "Invalid variant for boolean value.")},
      argument);
}
