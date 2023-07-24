#include "parseHelper.hpp"

bool hasCorrectSignature(
    const std::vector<std::pair<std::string_view, size_t>> &nameNumArgPairs,
    const fznparser::Constraint &constraint) {
  for (const auto &[name, numArgs] : nameNumArgPairs) {
    if (name == constraint.identifier() &&
        numArgs == constraint.arguments.size()) {
      return true;
    }
  }
  return false;
}

std::vector<VariableNode *> pruneAllDifferent(
    std::vector<VariableNode *> staticInputs) {
  // pruned[i] = <index, value> where index is the index of the static variable
  // with singleton domain {value}.
  std::vector<std::pair<size_t, Int>> pruned;
  pruned.reserve(staticInputs.size());

  for (size_t i = 0; i < staticInputs.size(); ++i) {
    for (const auto &[index, value] : pruned) {
      // remove all pruned values from the current variable:
      assert(index < i);
      staticInputs[i]->domain().removeValue(value);
    }
    if (!staticInputs[i]->domain().isConstant()) {
      continue;
    }
    // the variable has a singleton domain
    // Remove all occurrences of the value from previous static variables. Any
    // variable that gets a singleton domain is added to the pruned list.
    pruned.emplace_back(i, staticInputs[i]->domain().lowerBound());
    for (size_t p = pruned.size() - 1; p < pruned.size(); ++p) {
      const auto &[index, value] = pruned.at(p);
      for (size_t j = 0; j < index; j++) {
        const bool wasConstant = staticInputs[j]->domain().isConstant();
        staticInputs[j]->domain().removeValue(value);
        if (!wasConstant && staticInputs[j]->domain().isConstant()) {
          pruned.emplace_back(j, staticInputs[j]->domain().lowerBound());
        }
      }
    }
  }
  std::vector<std::reference_wrapper<VariableNode>> prunedVars(pruned.size());
  for (size_t i = 0; i < pruned.size(); ++i) {
    prunedVars[i] = staticInputs[pruned[i].first];
  }
  return prunedVars;
}

std::vector<Int> toIntVector(const std::vector<bool> &argument) {
  std::vector<Int> ints;
  ints.reserve(bools.size());
  std::transform(bools.begin(), bools.end(), std::back_inserter(ints),
                 [](const bool b) { return 1 - static_cast<Int>(b); });

  return ints;
}

/*
#define EMPTY_VARIANT_BRANCH(ParamType, ReturnType, message) \
  [](const ParamType &) {                                    \
    throw std::runtime_error(message);                       \
    return static_cast<ReturnType>(0);                       \
  }

#define DEFAULT_EMPTY_VARIANT_BRANCH(ReturnType, message) \
  EMPTY_VARIANT_BRANCH(auto, ReturnType, message)

using namespace fznparser;

template <typename T>
static invariantgraph::VariableNode *createVariableNode(
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
                         return createVariableNode(identifier, variableMap);
                       },
                       [&](const auto &value) {
                         return createVariableNode(value, variableMap);
                       }},
            element);
      });

  return nodes;
}

template <typename T>
static std::vector<invariantgraph::VariableNode *>
variableNodesFromVariableArray(const fznparser::VariableArray<T> &argument,
                               InvariantGraph &invariantGraph) {
  for (const auto &element : argument) {
    std::visit(
        [&](const auto &elem) {
          nodes.push_back(createVariableNode(elem, variableMap));
        },
        element);
  }

  return nodes;
}

std::vector<std::reference_wrapper<invariantgraph::VariableNode>>
mappedVariableVector(
    const fznparser::Model &model, const fznparser::Arg &argument,
    std::unordered_map<std::string_view, VariableNode> &variableMap) {
  if (std::holds_alternative<fznparser::BoolVarArray>(argument)) {
    const auto &array = std::get<fznparser::BoolVarArray>(argument);
    return variableNodesFromVariableArray<bool>(array, variableMap);
  } else if (std::holds_alternative<fznparser::IntVarArray>(argument)) {
    const auto &array = std::get<fznparser::IntVarArray>(argument);
    return variableNodesFromVariableArray<Int>(array, variableMap);
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
      overloaded{[&](bool b) { return createVariableNode(b, variableMap); },
                 [&](Int i) { return createVariableNode(i, variableMap); },
                 [&](const std::string_view &ident) {
                   return createVariableNode(ident, variableMap);
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
    const fznparser::Model &model, const FZNConstraint::ArrayArgument &array) {
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

std::vector<Int> integerVector(const fznparser::Model &model,
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

Int integerValue(const Model &model, const FZNConstraint::Argument &argument) {
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

fznparser::Set<Int> integerSet(const Model &model,
                               const FZNConstraint::Argument &argument) {
  if (std::holds_alternative<Set<Int>>(argument)) {
    return std::get<Set<Int>>(argument);
  }

  assert(std::holds_alternative<std::string_view>(argument));
  auto identifier = std::get<std::string_view>(argument);
  auto identifiable = *model.identify(identifier);
  assert(std::holds_alternative<Parameter>(identifiable));
  auto parameter = std::get<Parameter>(identifiable);
  assert(std::holds_alternative<SetOfIntParameter>(parameter));
  return std::get<SetOfIntParameter>(parameter).value;
}

std::vector<Int> boolVectorAsIntVector(
    const Model &model, const FZNConstraint::Argument &argument) {
  auto bools = std::visit<std::vector<bool>>(
      overloaded{
          [&](const std::string_view &identifier) {
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

bool booleanValue(const Model &model, const FZNConstraint::Argument &argument) {
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
*/