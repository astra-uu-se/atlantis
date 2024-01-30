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

}  // namespace atlantis::invariantgraph