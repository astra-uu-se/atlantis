#pragma once

#include <fznparser/model.hpp>
#include <optional>
#include <vector>

#include "invariantgraph/structure.hpp"
#include "utils/fznAst.hpp"

using FZNConstraint = fznparser::Constraint;
using FZNSearchVariable =
    std::variant<fznparser::IntVariable, fznparser::BoolVariable>;

std::vector<invariantgraph::VariableNode*> mappedVariableVector(
    const fznparser::FZNModel& model, const FZNConstraint::Argument& argument,
    const std::function<invariantgraph::VariableNode*(
        invariantgraph::MappableValue&)>& variableMap);

invariantgraph::VariableNode* mappedVariable(
    const FZNConstraint::Argument& argument,
    const std::function<invariantgraph::VariableNode*(
        invariantgraph::MappableValue&)>& variableMap);

std::vector<Int> integerVector(const fznparser::FZNModel& model,
                               const FZNConstraint::Argument& argument);

Int integerValue(const fznparser::FZNModel& model,
                 const FZNConstraint::Argument& argument);

fznparser::Set<Int> integerSet(const fznparser::FZNModel& model,
                               const FZNConstraint::Argument& argument);

bool definesVariable(const fznparser::Constraint& constraint,
                     const FZNSearchVariable& variable);
