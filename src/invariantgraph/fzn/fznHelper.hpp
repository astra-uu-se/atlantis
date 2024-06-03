#pragma once

#include <functional>
#include <fznparser/constraint.hpp>
#include <fznparser/types.hpp>
#include <fznparser/variables.hpp>
#include <string>
#include <typeinfo>
#include <unordered_set>
#include <variant>

#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/intCountNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/varIntCountNode.hpp"
#include "atlantis/types.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph::fzn {

std::string to_string(const std::type_info& t, bool isVar);

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

VarNodeId createCountNode(FznInvariantGraph& invariantGraph,
                          const std::shared_ptr<fznparser::IntVarArray>& inputs,
                          const fznparser::IntArg& needle);

VarNodeId createCountNode(FznInvariantGraph& invariantGraph,
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

#define FZN_CONSTRAINT_TYPE_CHECK(constraint, index, type, isVar)            \
  do {                                                                       \
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
    }                                                                        \
  } while (false);

#define FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, index, arrayType,        \
                                        isVarArray)                          \
  do {                                                                       \
    if (constraint.arguments().size() <= index) {                            \
      throw FznArgumentException("Constraint " + constraint.identifier() +   \
                                 " has too few arguments.");                 \
    }                                                                        \
    if (!std::holds_alternative<std::shared_ptr<arrayType>>(                 \
            constraint.arguments().at(index))) {                             \
      throw FznArgumentException(                                            \
          "Invalid argument for constraint " + constraint.identifier() +     \
          " at position" + std::to_string(index) + ": expected \"" +         \
          to_string(typeid(arrayType), isVarArray) + "\" but got \"" +       \
          to_string(typeid(constraint.arguments().at(index)), isVarArray) +  \
          '.');                                                              \
    }                                                                        \
    const auto& array = std::get<std::shared_ptr<arrayType>>(                \
        constraint.arguments().at(index));                                   \
    if (!isVarArray && !array->isParArray()) {                               \
      throw FznArgumentException(                                            \
          "Invalid argument for constraint " + constraint.identifier() +     \
          " at position" + std::to_string(index) + ": expected \"" +         \
          to_string(typeid(arrayType), isVarArray) + "\" but got \"" +       \
          to_string(typeid(constraint.arguments().at(index)), !isVarArray) + \
          '.');                                                              \
    }                                                                        \
  } while (false);

}  // namespace atlantis::invariantgraph::fzn
