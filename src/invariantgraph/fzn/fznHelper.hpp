#pragma once

#include <fznparser/constraint.hpp>
#include <string>

#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::invariantgraph {
class InvariantGraph;
class FznInvariantGraph;
}  // namespace atlantis::invariantgraph

namespace atlantis::invariantgraph::fzn {

std::string arg_type_to_string(const std::type_info& t, bool isVar);
std::string arg_type_to_string(const fznparser::Arg& arg);

bool hasSuffix(const std::string& str, const std::string& suffix);

// "_reif".size() == 5
bool constraintIdentifierIsReified(const fznparser::Constraint& constraint);

void verifyNumArguments(const fznparser::Constraint& constraint, size_t size);

std::vector<Int> getFixedValues(
    const std::shared_ptr<fznparser::IntVarArray>& intVarArray);

std::vector<bool> getFixedBoolValues(const InvariantGraph&,
                                     const std::vector<VarNodeId>&);

std::vector<bool> getFixedValues(
    const std::shared_ptr<fznparser::BoolVarArray>& boolVarArray);

std::vector<VarNodeId> retrieveUnfixedVarNodeIds(
    FznInvariantGraph&, const std::shared_ptr<fznparser::IntVarArray>&);

std::vector<VarNodeId> retrieveUnfixedVarNodeIds(
    FznInvariantGraph&, const std::shared_ptr<fznparser::BoolVarArray>&);

std::vector<VarNodeId> getUnfixedVarNodeIds(const InvariantGraph&,
                                            const std::vector<VarNodeId>&);

void verifyAllDifferent(
    const std::shared_ptr<fznparser::IntVarArray>& intVarArray);

[[nodiscard]] bool violatesAllEqual(
    const std::shared_ptr<fznparser::IntVarArray>& intVarArray);

VarNodeId createCountNode(FznInvariantGraph& graph,
                          const std::shared_ptr<fznparser::IntVarArray>& inputs,
                          const fznparser::IntArg& needle);

VarNodeId createCountNode(FznInvariantGraph& graph,
                          const std::shared_ptr<fznparser::IntVarArray>& inputs,
                          const fznparser::IntArg& needle,
                          const fznparser::IntArg& count);

void invertCoeffs(std::vector<Int>&);

std::pair<Int, Int> linBounds(const std::vector<Int>&,
                              const std::shared_ptr<fznparser::BoolVarArray>&);

std::pair<Int, Int> linBounds(const std::vector<Int>&,
                              const std::shared_ptr<fznparser::IntVarArray>&);

std::pair<Int, Int> linBounds(FznInvariantGraph&, const std::vector<Int>&,
                              const std::vector<VarNodeId>&);

template <typename T>
std::shared_ptr<T> getArgArray(const fznparser::Arg& argArray) {
  if (argArray.isEmptyArray()) {
    return std::make_shared<T>(std::string(""));
  }
  return std::get<std::shared_ptr<T>>(argArray);
}

#define FZN_CONSTRAINT_TYPE_CHECK(constraint, index, type, isVar)            \
  do {                                                                       \
    if ((index) == 0 ? (constraint).arguments().empty()                      \
                     : (constraint).arguments().size() <= (index)) {         \
      throw FznArgumentException("Constraint " + (constraint).identifier() + \
                                 " has too few arguments.");                 \
    }                                                                        \
    if (!std::holds_alternative<type>((constraint).arguments().at(index))) { \
      throw FznArgumentException(                                            \
          "Invalid argument for constraint " + (constraint).identifier() +   \
          " at position " + std::to_string(index) + ": expected \"" +        \
          arg_type_to_string(typeid(type), isVar) + "\" but got \"" +        \
          arg_type_to_string((constraint).arguments().at(index)) + "\".");   \
    }                                                                        \
  } while (false)

#define FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, index, arrayType,        \
                                        isVarArray)                          \
  do {                                                                       \
    if ((index) == 0 ? (constraint).arguments().empty()                      \
                     : (constraint).arguments().size() <= (index)) {         \
      throw FznArgumentException("Constraint " + (constraint).identifier() + \
                                 " has too few arguments.");                 \
    }                                                                        \
    if ((constraint).arguments().at(index).isEmptyArray()) {                 \
      break;                                                                 \
    }                                                                        \
    if (!std::holds_alternative<std::shared_ptr<arrayType>>(                 \
            (constraint).arguments().at(index))) {                           \
      throw FznArgumentException(                                            \
          "Invalid argument for constraint " + (constraint).identifier() +   \
          " at position " + std::to_string(index) + ": expected \"" +        \
          arg_type_to_string(typeid(arrayType), isVarArray) +                \
          "\" but got \"" +                                                  \
          arg_type_to_string((constraint).arguments().at(index)) + "\".");   \
    }                                                                        \
    const auto& array = std::get<std::shared_ptr<arrayType>>(                \
        (constraint).arguments().at(index));                                 \
    if (!(isVarArray) && !array->isParArray()) {                             \
      throw FznArgumentException(                                            \
          "Invalid argument for constraint " + (constraint).identifier() +   \
          " at position " + std::to_string(index) + ": expected \"" +        \
          arg_type_to_string(typeid(arrayType), isVarArray) +                \
          "\" but got \"" +                                                  \
          arg_type_to_string((constraint).arguments().at(index)) + "\".");   \
    }                                                                        \
  } while (false)

}  // namespace atlantis::invariantgraph::fzn
