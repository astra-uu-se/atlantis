#pragma once

#include <algorithm>
#include <fznparser/model.hpp>
#include <optional>
#include <variant>
#include <vector>

#include "invariantgraph/implicitConstraintNode.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "invariantgraph/varNode.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "utils/fznAst.hpp"
#include "utils/variant.hpp"

namespace invariantgraph {

std::vector<invariantgraph::VarNodeId>&& append(
    std::vector<invariantgraph::VarNodeId>&&, invariantgraph::VarNodeId,
    invariantgraph::VarNodeId);

std::vector<invariantgraph::VarNodeId>&& append(
    std::vector<invariantgraph::VarNodeId>&&, invariantgraph::VarNodeId);

std::vector<invariantgraph::VarNodeId> concat(
    const std::vector<invariantgraph::VarNodeId>&,
    const std::vector<invariantgraph::VarNodeId>&);

bool hasCorrectSignature(
    const std::vector<std::pair<std::string, size_t>>& nameNumArgPairs,
    const fznparser::Constraint& constraint);

std::vector<VarNodeId> pruneAllDifferentFree(
    InvariantGraph&, std::vector<VarNodeId> staticInputVarNodeIds);

std::vector<VarNodeId> pruneAllDifferentFixed(
    InvariantGraph&, std::vector<VarNodeId> staticInputVarNodeIds);

std::vector<Int> toIntVector(const std::vector<bool>& argument);

/*
std::vector<:VarNodeId> mappedVariableVector(
    const fznparser::Model& model, const FZNConstraint::Argument& argument,
    const std::function<:VarNodeId(
        :MappableValue&)>& variableMap);

:VarNodeId mappedVariable(
    const FZNConstraint::Argument& argument,
    const std::function<:VarNodeId(
        :MappableValue&)>& variableMap);

std::vector<Int> integerVector(const fznparser::Model& model,
                               const FZNConstraint::Argument& argument);

std::vector<Int> boolVectorAsIntVector(const fznparser::Model& model,
                                       const FZNConstraint::Argument& argument);

Int integerValue(const fznparser::Model& model,
                 const FZNConstraint::Argument& argument);

bool booleanValue(const fznparser::Model& model,
                  const FZNConstraint::Argument& argument);

fznparser::Set<Int> integerSet(const fznparser::Model& model,
                               const FZNConstraint::Argument& argument);

bool definesVariable(const fznparser::Constraint& constraint,
                     const FZNSearchVariable& variable);

*/

}  // namespace invariantgraph