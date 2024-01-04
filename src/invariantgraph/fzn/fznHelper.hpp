#pragma once

#include <functional>
#include <fznparser/constraint.hpp>
#include <fznparser/types.hpp>
#include <fznparser/variables.hpp>
#include <string>
#include <typeinfo>
#include <unordered_set>
#include <variant>

#include "exceptions/exceptions.hpp"
#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNodes/intCountNode.hpp"
#include "types.hpp"
#include "utils/domains.hpp"

namespace atlantis::invariantgraph::fzn {

std::string to_string(const std::type_info& t, bool isVar) {
  if (t == typeid(fznparser::BoolVar)) {
    return "var bool";
  } else if (t == typeid(fznparser::IntVar)) {
    return "var int";
  } else if (t == typeid(fznparser::IntVarArray)) {
    if (isVar) {
      return "array[int] of var bool";
    } else {
      return "array[int] of bool";
    }
  } else if (t == typeid(fznparser::IntVarArray)) {
    if (isVar) {
      return "array[int] of var int";
    } else {
      return "array[int] of int";
    }
  } else if (t == typeid(fznparser::IntSet)) {
    return "set of int";
  }
  return "[unknown type]";
}

// "_reif".size() == 5
bool constraintIdentifierIsReified(const fznparser::Constraint& constraint) {
  return constraint.identifier().size() >= size_t(5) &&
         0 == constraint.identifier().compare(
                  constraint.identifier().size() - size_t(5), size_t(5),
                  "_reif");
}

void verifyNumArguments(const fznparser::Constraint& constraint, size_t size) {
  if (constraint.arguments().size() != size) {
    throw FznArgumentException("Constraint " + constraint.identifier() +
                               " expects " + std::to_string(size) +
                               " arguments().");
  }
}

#define FZN_CONSTRAINT_TYPE_CHECK(constraint, index, type, isVar)          \
  if (constraint.arguments().size() <= index) {                            \
    throw FznArgumentException("Constraint " + constraint.identifier() +   \
                               " has too few arguments.");                 \
  }                                                                        \
  if (!std::holds_alternative<type>(constraint.arguments().at(index))) {   \
    throw FznArgumentException(                                            \
        "Invalid argument for constraint " + constraint.identifier() +     \
        " at position" + std::to_string(index) + ": expected \"" +         \
        to_string(typeid(type), isVar) + "\" but got \"" +                 \
        to_string(typeid(constraint.arguments().at(index)), isVar) + '.'); \
  }

std::vector<Int> getFixedValues(const fznparser::IntVarArray& intVarArray) {
  std::vector<Int> values;
  values.reserve(intVarArray.size());
  for (size_t i = 0; i < intVarArray.size(); ++i) {
    if (std::holds_alternative<Int>(intVarArray.at(i))) {
      values.emplace_back(std::get<Int>(intVarArray.at(i)));
      continue;
    }
    const auto& var = std::get<std::reference_wrapper<const fznparser::IntVar>>(
                          intVarArray.at(i))
                          .get();

    if (var.isFixed()) {
      values.emplace_back(var.lowerBound());
    }
  }
  return values;
}

std::vector<bool> getFixedValues(const fznparser::BoolVarArray& boolVarArray) {
  std::vector<bool> values;
  values.reserve(boolVarArray.size());
  for (size_t i = 0; i < boolVarArray.size(); ++i) {
    if (std::holds_alternative<bool>(boolVarArray.at(i))) {
      values.emplace_back(std::get<bool>(boolVarArray.at(i)));
      continue;
    }
    const auto& var =
        std::get<std::reference_wrapper<const fznparser::BoolVar>>(
            boolVarArray.at(i))
            .get();

    if (var.isFixed()) {
      values.emplace_back(var.lowerBound());
    }
  }
  return values;
}

void verifyAllDifferent(const fznparser::IntVarArray& intVarArray) {
  std::vector<Int> values = getFixedValues(intVarArray);
  std::unordered_set<Int> seenValues;
  seenValues.reserve(values.size());
  for (const Int val : values) {
    if (seenValues.contains(val)) {
      throw FznArgumentException(
          "The argument of constraint fzn_all_different has parameters with "
          "duplicate values.");
    }
    seenValues.insert(val);
  }
}

[[nodiscard]] bool violatesAllEqual(const fznparser::IntVarArray& intVarArray) {
  std::vector<Int> values = getFixedValues(intVarArray);
  for (size_t i = 1; i < values.size(); ++i) {
    if (values[i] != values[0]) {
      return true;
    }
  }
  return false;
}

VarNodeId createCountNode(FznInvariantGraph& invariantGraph,
                          const fznparser::IntVarArray& inputs,
                          const fznparser::IntArg& needle) {
  const SearchDomain domain(0, static_cast<Int>(inputs.size()));

  VarNodeId countVarNodeId = invariantGraph.createVarNode(domain, true, true);

  if (needle.isFixed()) {
    invariantGraph.addInvariantNode(std::make_unique<IntCountNode>(
        invariantGraph.createVarNodes(inputs, false), needle.toParameter(),
        countVarNodeId));
  } else {
    invariantGraph.addInvariantNode(std::make_unique<IntCountNode>(
        invariantGraph.createVarNodes(inputs, false),
        invariantGraph.createVarNode(needle.var(), false), countVarNodeId));
  }
  return countVarNodeId;
}

VarNodeId createCountNode(FznInvariantGraph& invariantGraph,
                          const fznparser::IntVarArray& inputs,
                          const fznparser::IntArg& needle,
                          const fznparser::IntArg& count) {
  VarNodeId countVarNodeId = invariantGraph.createVarNode(count, true);

  if (needle.isFixed()) {
    invariantGraph.addInvariantNode(std::make_unique<IntCountNode>(
        invariantGraph.createVarNodes(inputs, false), needle.toParameter(),
        countVarNodeId));
  } else {
    invariantGraph.addInvariantNode(std::make_unique<IntCountNode>(
        invariantGraph.createVarNodes(inputs, false),
        invariantGraph.createVarNode(needle.var(), false), countVarNodeId));
  }
  return countVarNodeId;
}

}  // namespace atlantis::invariantgraph::fzn