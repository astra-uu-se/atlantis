#pragma once

#include <algorithm>
#include <fznparser/model.hpp>
#include <optional>
#include <variant>
#include <vector>

#include "invariantgraph/implicitConstraintNode.hpp"
#include "invariantgraph/softConstraintNode.hpp"
#include "invariantgraph/variableDefiningNode.hpp"
#include "invariantgraph/variableNode.hpp"
#include "utils/fznAst.hpp"
#include "utils/variant.hpp"

namespace invariantgraph {

bool hasCorrectSignature(
    const std::vector<std::pair<std::string_view, size_t>>& nameNumArgPairs,
    const fznparser::Constraint& constraint);

std::vector<VariableNode*> pruneAllDifferent(
    std::vector<VariableNode*> staticInputs);

/*
std::vector<:VariableNode*> mappedVariableVector(
    const fznparser::Model& model, const FZNConstraint::Argument& argument,
    const std::function<:VariableNode*(
        :MappableValue&)>& variableMap);

:VariableNode* mappedVariable(
    const FZNConstraint::Argument& argument,
    const std::function<:VariableNode*(
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