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
  tmpVisited.resize(graph.numVariables() + 1, false);
  visited.resize(graph.numVariables() + 1, false);

  std::queue<VarId> reverseOrder;

  const std::function<void(VarId)> visit = [&](VarId id) {
    // tmp Mark current node
    assert(id < tmpVisited.size());
    tmpVisited[id] = true;
    // for each invariant id is input to
    for (const auto invariant : graph.listeningInvariants(id)) {
      // for each variable defined by that invariant
      for (const auto definedVariable : graph.variablesDefinedBy(invariant)) {
        assert(definedVariable < visited.size());
        if (visited[definedVariable]) {
          continue;
        }
        assert(definedVariable < tmpVisited.size());
        if (tmpVisited[definedVariable]) {
          throw PropagationGraphHasCycles(
              "var " + std::to_string(definedVariable) + " is part of cycle.");
        }
        visit(definedVariable);
      }
    }
    assert(id < visited.size());
    visited[id] = true;
    reverseOrder.push(id);
  };

  // Call visit on all top variables
  for (size_t varId = 0; varId < graph.numVariables() + 1; ++varId) {
    if (varId == NULL_ID) {  // Skip nullVar if any.
      continue;
    }
    if (graph.definingInvariant(varId).id == NULL_ID) {
      visit(VarId(varId));
    }
  }

  variablePosition.resize(graph.numVariables() + 1, 0);
  while (!reverseOrder.empty()) {
    assert(reverseOrder.front() < variablePosition.size());
    variablePosition[reverseOrder.front()] = reverseOrder.size();
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
  visited.resize(graph.numVariables() + 1, false);

  std::queue<VarId> reverseOrder;

  const std::function<void(VarId)> visit = [&](VarId varId) {
    // Mark current node
    assert(varId < visited.size());
    visited[varId] = true;
    // for each invariant id is a input to
    for (const auto invariant : graph.listeningInvariants(varId)) {
      // for each variable defined by that invariant
      for (const auto definedVariable : graph.variablesDefinedBy(invariant)) {
        assert(definedVariable < visited.size());
        if (visited[definedVariable]) {
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
  for (size_t varId = 0; varId < graph.numVariables() + 1; ++varId) {
    if (varId == NULL_ID) {  // Skip nullVar if any.
      continue;
    }
    if (graph.definingInvariant(varId).id == NULL_ID) {
      visit(VarId(varId));
    }
  }

  variablePosition.resize(graph.numVariables() + 1, 0);
  while (!reverseOrder.empty()) {
    assert(reverseOrder.front() < variablePosition.size());
    variablePosition[reverseOrder.front()] = reverseOrder.size();
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
  visited.resize(graph.numVariables() + 1, false);

  std::vector<int> position;
  position.resize(graph.numVariables() + 1, 0);

  std::queue<VarId> reverseOrder;

  const std::function<void(VarId, int)> visit = [&](VarId varId, int depth) {
    // Mark current node
    assert(varId < visited.size());
    visited[varId] = true;
    assert(varId < position.size());
    position[varId] = depth;
    // std::cout << "Visiting " << varId << " at depth " << depth << std::endl;
    // for each invariant id is a input to
    for (const auto invariantId : graph.listeningInvariants(varId)) {
      // for each variable defined by that invariant
      for (const auto definedVariable : graph.variablesDefinedBy(invariantId)) {
        assert(definedVariable < visited.size());
        if (visited[definedVariable]) {
          // Ignore nodes that have been visited before
          // This also breaks cycles.
          // std::cout << "Variable " << definedVariable
          //           << " has already been visitied in this recursion"
          //           << std::endl;
          continue;
        }
        assert(definedVariable < position.size());
        if (position[definedVariable] > depth) {
          // Ignore variable that is already at a lower level.
          // std::cout << "Variable " << definedVariable
          //           << " is already at depth " <<
          //           position[definedVariable]
          //           << std::endl;
          continue;
        }
        visit(definedVariable, depth + 1);
      }
    }
  };

  // Call visit on all top variables
  for (size_t varId = 0; varId < graph.numVariables() + 1; ++varId) {
    if (varId == NULL_ID) {  // Skip nullVar if any.
      continue;
    }

    visited.assign(graph.numVariables() + 1, false);
    if (graph.definingInvariant(varId).id == NULL_ID) {
      visit(VarId(varId), 0);
    }
  }

  // int max = 0;
  // for (int v : position) {
  //   max = std::max(max, v);
  // }
  // std::vector<int> count;
  // count.resize(max+1, 0);
  variablePosition.resize(graph.numVariables() + 1, 0);
  assert(position.size() == variablePosition.size());
  for (size_t i = 0; i < graph.numVariables() + 1; ++i) {
    variablePosition[i] = position[i];
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
  invariantPosition.resize(graph.numInvariants() + 1, 0);
  for (size_t i = 0; i < graph.numInvariants() + 1; ++i) {
    if (i == NULL_ID) {  // Skip nullVar if any.
      continue;
    }
    const InvariantId invariantId(i);
    size_t position = 0;
    for (const auto definedVariable : graph.variablesDefinedBy(invariantId)) {
      assert(definedVariable < variablePosition.size());
      position = std::max<size_t>(position, variablePosition[definedVariable]);
    }
    assert(invariantId < invariantPosition.size());
    invariantPosition[invariantId] = position;
  }
}
