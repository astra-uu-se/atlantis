#pragma once

#include <vector>

#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::invariantgraph {
class InvariantGraph;

std::vector<VarNodeId>&& append(std::vector<VarNodeId>&&, VarNodeId, VarNodeId);

std::vector<VarNodeId>&& append(std::vector<VarNodeId>&&, VarNodeId);

std::vector<VarNodeId> concat(const std::vector<VarNodeId>&,
                              const std::vector<VarNodeId>&);

std::vector<VarNodeId> pruneAllDifferentFree(
    InvariantGraph&, const std::vector<VarNodeId>& staticInputVarNodeIds);

std::vector<VarNodeId> pruneAllDifferentFixed(
    InvariantGraph&, const std::vector<VarNodeId>& staticInputVarNodeIds);

std::vector<Int> toIntVector(const std::vector<bool>& argument);

/**
 * This potentially rearranges the vector.
 */
bool removeFirstOccurrence(std::vector<size_t>&, size_t);

std::vector<ConstraintVarId> toConstraintVarIds(const InvariantGraph&, const std::vector<VarNodeId>&);

}  // namespace atlantis::invariantgraph
