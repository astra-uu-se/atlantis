#include <functional>
#include <iostream>
#include <queue>

#include "exceptions/exceptions.hpp"
#include "propagation/propagationGraph.hpp"
/**
 * Throws an exception if the graph has cycles.
 * Uses different key-domains for variables and invariants.
 */
void PropagationGraph::Topology::computeNoCycles() {
  std::vector<bool> tmpVisited;
  std::vector<bool> visited;
  tmpVisited.resize(graph.getNumVariables() + 1, false);
  visited.resize(graph.getNumVariables() + 1, false);

  std::queue<VarId> reverseOrder;

  const std::function<void(VarId)> visit = [&](VarId id) {
    // tmp Mark current node
    tmpVisited.at(id) = true;
    // for each invariant id is input to
    for (auto invariant : graph.getListeningInvariants(id)) {
      // for each variable defined by that invariant
      for (auto definedVariable : graph.getVariablesDefinedBy(invariant)) {
        if (visited.at(definedVariable)) {
          continue;
        }
        if (tmpVisited.at(definedVariable)) {
          throw PropagationGraphHasCycles(
              "var " + std::to_string(definedVariable) + " is part of cycle.");
        }
        visit(definedVariable);
      }
    }
    visited.at(id) = true;
    reverseOrder.push(id);
  };

  // Call visit on all top variables
  for (size_t varId = 0; varId < graph.getNumVariables() + 1; ++varId) {
    if (varId == NULL_ID) {  // Skip nullVar if any.
      continue;
    }
    if (graph.getDefiningInvariant(varId).id == NULL_ID) {
      visit(VarId(varId));
    }
  }

  variablePosition.resize(graph.getNumVariables() + 1, 0);
  while (!reverseOrder.empty()) {
    variablePosition.at(reverseOrder.front()) = reverseOrder.size();
    reverseOrder.pop();
  }
  computeInvariantFromVariables();
}

/**
 * Computes a topological sort from a dependency graph with cycles by
 * non-deterministically ignoring one edge in each cycle.
 * This means that there will be an order within cycles.
 *
 * Gives different key-domains to Variables and invariants.
 * That is, the key of invariants cannot be compared with variables.
 */
void PropagationGraph::Topology::computeWithCycles() {
  std::vector<bool> visited;
  visited.resize(graph.getNumVariables() + 1, false);

  std::queue<VarId> reverseOrder;

  const std::function<void(VarId)> visit = [&](VarId varId) {
    // Mark current node
    visited.at(varId) = true;
    // for each invariant id is a input to
    for (auto invariant : graph.getListeningInvariants(varId)) {
      // for each variable defined by that invariant
      for (auto definedVariable : graph.getVariablesDefinedBy(invariant)) {
        if (visited.at(definedVariable)) {
          // Ignore nodes that have been visited before
          // This also breaks cycles.
          continue;
        }
        visit(definedVariable);
      }
    }
    reverseOrder.push(varId);
  };

  // Call visit on all top variables
  for (size_t varId = 0; varId < graph.getNumVariables() + 1; ++varId) {
    if (varId == NULL_ID) {  // Skip nullVar if any.
      continue;
    }
    if (graph.getDefiningInvariant(varId).id == NULL_ID) {
      visit(VarId(varId));
    }
  }

  variablePosition.resize(graph.getNumVariables() + 1, 0);
  while (!reverseOrder.empty()) {
    variablePosition.at(reverseOrder.front()) = reverseOrder.size();
    reverseOrder.pop();
  }
  computeInvariantFromVariables();
}

/**
 * Computes a topological sort from a dependency graph with cycles by
 * non-deterministically ignoring one edge in each cycle.
 * This means that there will be an order within cycles.
 *
 * Gives different key-domains to Variables and invariants.
 * That is, the key of invariants cannot be compared with variables.
 *
 * Variables that are in the same propagation layer will (most of the time)
 * share key-value.
 */
void PropagationGraph::Topology::computeLayersWithCycles() {
  std::vector<bool> visited;
  visited.resize(graph.getNumVariables() + 1, false);

  std::vector<int> position;
  position.resize(graph.getNumVariables() + 1, 0);

  std::queue<VarId> reverseOrder;

  std::function<void(VarId, int)> visit = [&](VarId varId, int depth) {
    // Mark current node
    visited.at(varId) = true;
    position.at(varId) = depth;
    // std::cout << "Visiting " << varId << " at depth " << depth << std::endl;
    // for each invariant id is a input to
    for (auto invariantId : graph.getListeningInvariants(varId)) {
      // for each variable defined by that invariant
      for (auto definedVariable : graph.getVariablesDefinedBy(invariantId)) {
        if (visited.at(definedVariable)) {
          // Ignore nodes that have been visited before
          // This also breaks cycles.
          // std::cout << "Variable " << definedVariable
          //           << " has already been visitied in this recursion"
          //           << std::endl;
          continue;
        }
        if (position.at(definedVariable) > depth) {
          // Ignore variable that is already at a lower level.
          // std::cout << "Variable " << definedVariable
          //           << " is already at depth " <<
          //           position.at(definedVariable)
          //           << std::endl;
          continue;
        }
        visit(definedVariable, depth + 1);
      }
    }
  };

  // Call visit on all top variables
  for (size_t varId = 0; varId < graph.getNumVariables() + 1; ++varId) {
    if (varId == NULL_ID) {  // Skip nullVar if any.
      continue;
    }

    visited.assign(graph.getNumVariables() + 1, false);
    if (graph.getDefiningInvariant(varId).id == NULL_ID) {
      visit(VarId(varId), 0);
    }
  }

  // int max = 0;
  // for (int v : position) {
  //   max = std::max(max, v);
  // }
  // std::vector<int> count;
  // count.resize(max+1, 0);
  variablePosition.resize(graph.getNumVariables() + 1, 0);
  for (size_t i = 0; i < graph.getNumVariables() + 1; ++i) {
    variablePosition.at(i) = position.at(i);
    // count.at(position.at(i))++;
  }

  // for (size_t i = 0; i < count.size(); ++i) {
  //   std::cout << i << ": " << count.at(i) << std::endl;
  // }
  computeInvariantFromVariables();
}

/**
 * Computes a topology from the current topology of variables.
 * Notes that this gives different key-domains to Variables and invariants.
 */
void PropagationGraph::Topology::computeInvariantFromVariables() {
  invariantPosition.resize(graph.getNumInvariants() + 1, 0);
  for (size_t i = 0; i < graph.getNumInvariants() + 1; ++i) {
    if (i == NULL_ID) {  // Skip nullVar if any.
      continue;
    }
    auto invariantId = InvariantId(i);
    size_t position = 0;
    for (auto definedVariable : graph.getVariablesDefinedBy(invariantId)) {
      position =
          std::max<size_t>(position, variablePosition.at(definedVariable));
    }
    invariantPosition.at(invariantId) = position;
  }
}
