#include "parseHelper.hpp"

#include "invariantgraph/invariantGraph.hpp"

namespace atlantis::invariantgraph {

bool hasCorrectSignature(
    const std::vector<std::pair<std::string, size_t>> &nameNumArgPairs,
    const fznparser::Constraint &constraint) {
  for (const auto &[name, numArgs] : nameNumArgPairs) {
    if (name == constraint.identifier() &&
        numArgs == constraint.arguments().size()) {
      return true;
    }
  }
  return false;
}

std::vector<invariantgraph::VarNodeId> &&append(
    std::vector<invariantgraph::VarNodeId> &&vars,
    invariantgraph::VarNodeId fst, invariantgraph::VarNodeId snd) {
  if (fst != NULL_NODE_ID) {
    vars.emplace_back(fst);
  }
  if (snd != NULL_NODE_ID) {
    vars.emplace_back(snd);
  }
  return std::move(vars);
}

std::vector<invariantgraph::VarNodeId> &&append(
    std::vector<invariantgraph::VarNodeId> &&vars,
    invariantgraph::VarNodeId var) {
  if (var != NULL_NODE_ID) {
    vars.emplace_back(var);
  }
  return std::move(vars);
}

std::vector<invariantgraph::VarNodeId> concat(
    const std::vector<invariantgraph::VarNodeId> &fst,
    const std::vector<invariantgraph::VarNodeId> &snd) {
  std::vector<invariantgraph::VarNodeId> res;
  res.reserve(fst.size() + snd.size());
  res.insert(res.end(), fst.begin(), fst.end());
  res.insert(res.end(), snd.begin(), snd.end());
  return res;
}

static std::vector<std::pair<size_t, Int>> allDifferent(
    InvariantGraph &invariantGraph, std::vector<VarNodeId> inputs) {
  // pruned[i] = <index, value> where index is the index of the static
  // variable with singleton domain {value}.
  std::vector<std::pair<size_t, Int>> fixed;
  fixed.reserve(inputs.size());

  for (size_t i = 0; i < inputs.size(); ++i) {
    for (const auto &[index, value] : fixed) {
      // remove all fixed values from the current variable:
      assert(index < i);
      invariantGraph.varNode(inputs[i]).domain().removeValue(value);
    }
    if (!invariantGraph.varNode(inputs[i]).domain().isFixed()) {
      continue;
    }
    // the variable has a singleton domain
    // Remove all occurrences of the value from previous static variables. Any
    // variable that gets a singleton domain is added to the fixed list.
    fixed.emplace_back(i, invariantGraph.varNode(inputs[i]).val());
    for (size_t p = fixed.size() - 1; p < fixed.size(); ++p) {
      const auto &[index, value] = fixed.at(p);
      for (size_t j = 0; j < index; j++) {
        const bool wasConstant =
            invariantGraph.varNode(inputs[j]).domain().isFixed();
        invariantGraph.varNode(inputs[j]).domain().removeValue(value);
        if (!wasConstant &&
            invariantGraph.varNode(inputs[j]).domain().isFixed()) {
          fixed.emplace_back(j, invariantGraph.varNode(inputs[j]).val());
        }
      }
    }
  }
  return fixed;
}

std::vector<VarNodeId> pruneAllDifferentFree(InvariantGraph &invariantGraph,
                                             std::vector<VarNodeId> inputs) {
  const auto fixed = allDifferent(invariantGraph, inputs);
  std::vector<bool> isFree(inputs.size(), true);
  for (const auto &[index, _] : fixed) {
    isFree[index] = false;
  }
  std::vector<VarNodeId> freeVars;
  freeVars.reserve(inputs.size() - fixed.size());
  for (size_t i = 0; i < inputs.size(); ++i) {
    if (isFree[i]) {
      freeVars.push_back(inputs[i]);
    }
  }
  return freeVars;
}

std::vector<VarNodeId> pruneAllDifferentFixed(InvariantGraph &invariantGraph,
                                              std::vector<VarNodeId> inputs) {
  const auto fixed = allDifferent(invariantGraph, inputs);
  std::vector<bool> isFree(inputs.size(), true);
  for (const auto &[index, _] : fixed) {
    isFree[index] = false;
  }
  std::vector<VarNodeId> freeVars;
  freeVars.reserve(inputs.size() - fixed.size());
  for (size_t i = 0; i < inputs.size(); ++i) {
    if (!isFree[i]) {
      freeVars.push_back(inputs[i]);
    }
  }
  return freeVars;
}

std::vector<Int> toIntVector(const std::vector<bool> &argument) {
  std::vector<Int> ints;
  ints.reserve(argument.size());
  std::transform(argument.begin(), argument.end(), std::back_inserter(ints),
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
static invariantgraph::VarNode *createVarNode(
    T element, const std::function<invariantgraph::VarNode *(
                   invariantgraph::MappableValue &)> &varFactory) {
  invariantgraph::MappableValue value(element);
  return varFactory(value);
}

static std::vector<invariantgraph::VarNode *> varNodesFromLiteral(
    const FZNConstraint::ArrayArgument &argument,
    const std::function<invariantgraph::VarNode *(
        invariantgraph::MappableValue &)> &varMap) {
  std::vector<invariantgraph::VarNode *> nodes;
  nodes.reserve(argument.size());

  std::transform(
      argument.begin(), argument.end(), std::back_inserter(nodes),
      [&](const auto &element) {
        return std::visit<invariantgraph::VarNode *>(
            overloaded{EMPTY_VARIANT_BRANCH(
                           Set<Int>, invariantgraph::VarNode *,
                           "Cannot parse Set<Int> as a variable node."),
                       [&](const Identifier &identifier) {
                         return createVarNode(identifier, varMap);
                       },
                       [&](const auto &value) {
                         return createVarNode(value, varMap);
                       }},
            element);
      });

  return nodes;
}

template <typename T>
static std::vector<invariantgraph::VarNode *>
varNodesFromVarArray(const fznparser::VarArray<T> &argument,
                               InvariantGraph &invariantGraph) {
  for (const auto &element : argument) {
    std::visit(
        [&](const auto &elem) {
          nodes.push_back(createVarNode(elem, varMap));
        },
        element);
  }

  return nodes;
}

std::vector<std::reference_wrapper<invariantgraph::VarNode>>
mappedVarVector(
    const fznparser::Model &model, const fznparser::Arg &argument,
    std::unordered_map<std::string, VarNode> &varMap) {
  if (std::holds_alternative<fznparser::BoolVarArray>(argument)) {
    const auto &array = std::get<fznparser::BoolVarArray>(argument);
    return varNodesFromVarArray<bool>(array, varMap);
  } else if (std::holds_alternative<fznparser::IntVarArray>(argument)) {
    const auto &array = std::get<fznparser::IntVarArray>(argument);
    return varNodesFromVarArray<Int>(array, varMap);
  }

  throw std::runtime_error(
      "Expected constraint argument to be an array or an identifier for an "
      "array.");
}

invariantgraph::VarNode *mappedVar(
    const fznparser::Constraint::Argument &argument,
    const std::function<invariantgraph::VarNode *(
        invariantgraph::MappableValue &)> &varMap) {
  return std::visit<invariantgraph::VarNode *>(
      overloaded{[&](bool b) { return createVarNode(b, varMap); },
                 [&](Int i) { return createVarNode(i, varMap); },
                 [&](const std::string &ident) {
                   return createVarNode(ident, varMap);
                 },
                 DEFAULT_EMPTY_VARIANT_BRANCH(
                     invariantgraph::VarNode *,
                     "Unexpected variant for variable node ptr.")},
      argument);
}

template <typename T>
static T valueFromParameter(const Parameter &param) {
  return std::get<BaseParameter<T>>(param).value;
}

template <typename T>
static std::vector<T> valueLiteralVector(
    const fznparser::Model &model, const FZNConstraint::ArrayArgument &array)
{ std::vector<T> values; values.reserve(array.size());

  for (const auto &element : array) {
    auto value = std::visit<T>(
        overloaded{
            [](T v) { return v; },
            [&](const Identifier &identifier) {
              auto parameter =
std::get<Parameter>(*model.identify(identifier)); return
valueFromParameter<T>(parameter);
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

Int integerValue(const Model &model, const FZNConstraint::Argument &argument)
{ return std::visit<Int>( overloaded{[](Int value) { return value; },
                 [&](const Identifier &identifier) {
                   auto parameter =
                       std::get<Parameter>(*model.identify(identifier));
                   return valueFromParameter<Int>(parameter);
                 },
                 DEFAULT_EMPTY_VARIANT_BRANCH(
                     Int, "Invalid variant for an integer value.")},
      argument);
}

bool definesVar(const FZNConstraint &constraint,
                     const FznSearchVar &var) {
  auto definedVarId = definedVar(constraint);
  if (!definedVarId) {
    return false;
  }

  return std::visit<bool>(
      [&](const auto &var) { return definedVarId == var.name; },
var);
}

fznparser::Set<Int> integerSet(const Model &model,
                               const FZNConstraint::Argument &argument) {
  if (std::holds_alternative<Set<Int>>(argument)) {
    return std::get<Set<Int>>(argument);
  }

  assert(std::holds_alternative<std::string>(argument));
  auto identifier = std::get<std::string>(argument);
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
          [&](const std::string &identifier) {
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

bool booleanValue(const Model &model, const FZNConstraint::Argument &argument)
{ return std::visit<bool>( overloaded{[](bool value) { return value; },
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

}  // namespace atlantis::invariantgraph