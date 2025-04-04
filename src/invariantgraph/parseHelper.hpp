#pragma once

#include <vector>

#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::invariantgraph {
class IInvariantGraph;

std::vector<VarNodeId>&& append(std::vector<VarNodeId>&&, VarNodeId, VarNodeId);

std::vector<VarNodeId>&& append(std::vector<VarNodeId>&&, VarNodeId);

std::vector<VarNodeId> concat(const std::vector<VarNodeId>&,
                              const std::vector<VarNodeId>&);

std::vector<VarNodeId> pruneAllDifferentFree(
    IInvariantGraph&, std::vector<VarNodeId> staticInputVarNodeIds);

std::vector<VarNodeId> pruneAllDifferentFixed(
    IInvariantGraph&, std::vector<VarNodeId> staticInputVarNodeIds);

std::vector<Int> toIntVector(const std::vector<bool>& argument);

}  // namespace atlantis::invariantgraph
