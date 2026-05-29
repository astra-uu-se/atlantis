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

std::vector<ConstraintVarId> toConstraintVarIds(const InvariantGraph&,
                                                const std::vector<VarNodeId>&);

void postAllEqualOnReplacedVars(
    InvariantGraph& invariantGraph,
    std::vector<std::pair<VarNodeId, VarNodeId>>&& replacedVarNodeIds);

std::pair<std::vector<VarNodeId>, std::vector<size_t>> gccUpdateState(
    const InvariantGraph& invariantGraph, const std::vector<VarNodeId>& inputs,
    const std::vector<Int>& cover);

std::pair<std::vector<VarNodeId>, std::vector<size_t>> gccUpdateState(
    const InvariantGraph& invariantGraph, const std::vector<VarNodeId>& inputs,
    const std::vector<Int>& cover, std::vector<Int>& lowerBounds,
    std::vector<Int>& upperBounds);

}  // namespace atlantis::invariantgraph
