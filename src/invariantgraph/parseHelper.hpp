#pragma once

#include <algorithm>
#include <fznparser/model.hpp>
#include <optional>
#include <variant>
#include <vector>

#include "invariantgraph/implicitConstraintNode.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "invariantgraph/varNode.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "utils/fznAst.hpp"
#include "utils/variant.hpp"

namespace atlantis::invariantgraph {

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
using FznSearchVar = std::variant<fznparser::IntVar, fznparser::BoolVar>;

std::vector<:VarNodeId> mappedVarVector(
    const fznparser::Model& model, const FZNConstraint::Argument& argument,
    const std::function<:VarNodeId(
        :MappableValue&)>& varMap);

:VarNodeId mappedVar(
    const FZNConstraint::Argument& argument,
    const std::function<:VarNodeId(
        :MappableValue&)>& varMap);

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

bool definesVar(const fznparser::Constraint& constraint,
                     const FznSearchVar& var);

*/

}  // namespace atlantis::invariantgraph