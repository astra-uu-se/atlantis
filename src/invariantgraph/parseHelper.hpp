#pragma once

#include <vector>

#include "atlantis/invariantgraph/iInvariantGraph.hpp"

namespace atlantis::invariantgraph {

std::vector<invariantgraph::VarNodeId>&& append(
    std::vector<invariantgraph::VarNodeId>&&, invariantgraph::VarNodeId,
    invariantgraph::VarNodeId);

std::vector<invariantgraph::VarNodeId>&& append(
    std::vector<invariantgraph::VarNodeId>&&, invariantgraph::VarNodeId);

std::vector<invariantgraph::VarNodeId> concat(
    const std::vector<invariantgraph::VarNodeId>&,
    const std::vector<invariantgraph::VarNodeId>&);

std::vector<VarNodeId> pruneAllDifferentFree(
    IInvariantGraph&, std::vector<VarNodeId> staticInputVarNodeIds);

std::vector<VarNodeId> pruneAllDifferentFixed(
    IInvariantGraph&, std::vector<VarNodeId> staticInputVarNodeIds);

std::vector<Int> toIntVector(const std::vector<bool>& argument);

}  // namespace atlantis::invariantgraph
