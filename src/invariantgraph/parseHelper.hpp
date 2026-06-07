#pragma once

#include <vector>

#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/propagation/propagationGraph.hpp"
#include "atlantis/sortedUniqueVector.hpp"

namespace atlantis::propagation {
class SolverBase;
}
namespace atlantis::invariantgraph {
class SolverMapping;
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

std::pair<std::vector<VarNodeId>, SortedUniqueVector> gccUpdateState(
    const InvariantGraph& invariantGraph, const std::vector<VarNodeId>& inputs,
    const std::vector<Int>& cover);

std::pair<std::vector<VarNodeId>, SortedUniqueVector> gccUpdateState(
    const InvariantGraph& invariantGraph, const std::vector<VarNodeId>& inputs,
    const std::vector<Int>& cover, std::vector<Int>& lowerBounds,
    std::vector<Int>& upperBounds);

propagation::VarViewId solverConstRelation(propagation::SolverBase& solver,
                                           propagation::VarViewId lhs, Int rhs,
                                           RelationType relType,
                                           bool shouldHold = true);

void makeSolverRelation(propagation::SolverBase& solver,
                        propagation::VarViewId lhs, RelationType relType,
                        propagation::VarViewId rhs,
                        propagation::VarViewId violation, bool shouldHold);

void makeSolverBoolRelation(propagation::SolverBase& solver,
                            propagation::VarViewId lhs, RelationType relType,
                            propagation::VarViewId rhs,
                            propagation::VarViewId violation, bool shouldHold);

bool violToBool(Int violation);

std::vector<bool> violToBool(const std::vector<Int>& violations);

std::vector<std::vector<bool>> violToBool(
    const std::vector<std::vector<Int>>& violations);

Int boolToViol(bool b);

std::vector<Int> boolToViol(const std::vector<bool>& bools);

std::vector<std::vector<Int>> boolToViol(
    const std::vector<std::vector<bool>>& bools);

}  // namespace atlantis::invariantgraph
