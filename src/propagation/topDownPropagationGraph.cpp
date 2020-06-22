#include "propagation/topDownPropagationGraph.hpp"


#include "exceptions/exceptions.hpp"

TopDownPropagationGraph::TopDownPropagationGraph(size_t expectedSize)
    : PropagationGraph(expectedSize),
      m_topology(*this),
      m_modifiedVariables(PriorityCmp(m_topology)) {
  m_varsLastChange.reserve(expectedSize);

  m_varsLastChange.push_back(NULL_TIMESTAMP);
}

void TopDownPropagationGraph::notifyMaybeChanged(const Timestamp& t, VarId id) {
  if (m_varsLastChange.at(id) == t || !isActive(t, id)) {
    return;
  }
  m_varsLastChange[id] = t;

  m_modifiedVariables.push(id);
}

VarId TopDownPropagationGraph::getNextStableVariable([
    [maybe_unused]] const Timestamp& t) {
  if (m_modifiedVariables.empty()) {
    return VarId(NULL_ID);
  }
  VarId nextVar = m_modifiedVariables.top();
  m_modifiedVariables.pop();
  // Due to notifyMaybeChanged, all variables in the queue are "active".
  return nextVar;
}

void TopDownPropagationGraph::registerVar(VarId id) {
  PropagationGraph::registerVar(id);  // call parent implementation
  assert(id.id == m_varsLastChange.size());
  m_varsLastChange.push_back(NULL_TIMESTAMP);
}


/**
 * Assumes that the graph has no cycles. When there are cycles this code does
 * not terminate.
 *
 * Also This implementation is very inefficient, I think it is O(VE+E).  see
 * https://en.wikipedia.org/wiki/Topological_sorting for better versions.
 */
void TopDownPropagationGraph::Topology::computeNoCycles() {
  m_variablePosition.resize(graph.m_numVariables + 1, 0);
  m_invariantPosition.resize(graph.m_numInvariants + 1, 0);

  // We only keep track of a variable frontier. Invariants are visited on the
  // fly.
  std::queue<VarId> frontier;

  // Find the top level variables that are not defined by anything.
  // todo: it would be cleaning if we maintained a list of all added varIDs but
  // it is more efficient just to recreated based on the total number.
  for (size_t i = 0; i < graph.m_numVariables + 1; i++) {
    if (i == NULL_ID) {  // Skip nullVar if any.
      continue;
    }
    if (graph.m_definingInvariant.at(i).id == NULL_ID) {
      frontier.emplace(VarId(i));
      m_variablePosition.at(i) = 0;  // Variable is at top level.
    }
  }

  while (!frontier.empty()) {
    VarId currentNode = frontier.front();
    frontier.pop();
    size_t nodeDepth = m_variablePosition.at(currentNode);
    for (auto dependencyData : graph.m_listeningInvariants.at(currentNode)) {
      InvariantId invariant = dependencyData.id;
      m_invariantPosition.at(invariant) =
          std::max<size_t>(nodeDepth + 1, m_invariantPosition.at(invariant));
      for (auto dependentVariable :
           graph.m_variablesDefinedByInvariant.at(invariant)) {
        m_variablePosition.at(dependentVariable) = std::max<size_t>(
            nodeDepth + 2, m_variablePosition.at(dependentVariable));
        frontier.push(dependentVariable);
      }
    }
  }
}

/**
 * Throws an exception if the graph has cycles.
 * Uses different key-domains for variables and invariants.
 */
void TopDownPropagationGraph::Topology::computeNoCyclesException() {
  std::vector<bool> tmpVisited;
  std::vector<bool> visited;
  tmpVisited.resize(graph.m_numVariables + 1, false);
  visited.resize(graph.m_numVariables + 1, false);

  std::queue<VarId> reverseOrder;

  std::function<void(VarId)> visit = [&](VarId id) {
    // tmp Mark current node
    tmpVisited.at(id) = true;
    // for each dependent invariant
    for (auto invariant : graph.m_listeningInvariants.at(id)) {

      // for each variable defined by that invariant
      for (auto dependentVariable :
           graph.m_variablesDefinedByInvariant.at(invariant)) {
        if (visited.at(dependentVariable)) {
          continue;
        } else if (tmpVisited.at(dependentVariable)) {
          throw PropagationGraphHasCycles("var " +
                                          std::to_string(dependentVariable) +
                                          " is part of cycle.");
        }
        visit(dependentVariable);
      }
    }
    visited.at(id) = true;
    reverseOrder.push(id);
  };

  // Call visit on all top variables
  for (size_t i = 0; i < graph.m_numVariables + 1; i++) {
    if (i == NULL_ID) {  // Skip nullVar if any.
      continue;
    }
    if (graph.m_definingInvariant.at(i).id == NULL_ID) {
      visit(VarId(i));
    }
  }

  m_variablePosition.resize(graph.m_numVariables + 1, 0);
  while (!reverseOrder.empty()) {
    m_variablePosition.at(reverseOrder.front()) = reverseOrder.size();
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
void TopDownPropagationGraph::Topology::computeWithCycles() {
  std::vector<bool> visited;
  visited.resize(graph.m_numVariables + 1, false);

  std::queue<VarId> reverseOrder;

  std::function<void(VarId)> visit = [&](VarId id) {
    // Mark current node
    visited.at(id) = true;
    // for each dependent invariant
    for (auto invariant : graph.m_listeningInvariants.at(id)) {

      // for each variable defined by that invariant
      for (auto dependentVariable :
           graph.m_variablesDefinedByInvariant.at(invariant)) {
        if (visited.at(dependentVariable)) {
          // Ignore nodes that have been visited before
          // This also breaks cycles.
          continue;
        }
        visit(dependentVariable);
      }
    }
    reverseOrder.push(id);
  };

  // Call visit on all top variables
  for (size_t i = 0; i < graph.m_numVariables + 1; i++) {
    if (i == NULL_ID) {  // Skip nullVar if any.
      continue;
    }
    if (graph.m_definingInvariant.at(i).id == NULL_ID) {
      visit(VarId(i));
    }
  }

  m_variablePosition.resize(graph.m_numVariables + 1, 0);
  while (!reverseOrder.empty()) {
    m_variablePosition.at(reverseOrder.front()) = reverseOrder.size();
    reverseOrder.pop();
  }
  computeInvariantFromVariables();
}

void TopDownPropagationGraph::Topology::computeBundleCycles() {
  m_variablePosition.resize(graph.m_numVariables + 1, 0);

  std::vector<bool> isPartOfCycle;
  isPartOfCycle.resize(graph.m_numVariables + 1, false);

  std::vector<bool> tmpMark;
  tmpMark.resize(graph.m_numVariables + 1, false);

  std::function<bool(VarId, VarId)> detectCycle = [&](VarId target,
                                                      VarId current) {
    if (tmpMark.at(current) || isPartOfCycle.at(target)) {
      return false;
    }
    tmpMark.at(current) = true;
    bool inCycle = false;
    for (auto invariant : graph.m_listeningInvariants.at(current)) {
      // for each variable defined by that invariant
      for (auto dependentVariable :
           graph.m_variablesDefinedByInvariant.at(invariant)) {
        if (dependentVariable.id == target.id) {
          isPartOfCycle.at(target) = true;
          inCycle = true;
        } else {
          inCycle |= detectCycle(target, dependentVariable);
        }
      }
    }
    isPartOfCycle.at(current) = isPartOfCycle.at(current) || inCycle;
    // Remove mark from node so that it can be visited in different recursion.
    tmpMark.at(current) = false;
    return inCycle;
  };

  std::function<void(VarId, size_t)> visit = [&](VarId id, size_t pos) {
    // This ensure that position = max(currentPos, pos)
    if (m_variablePosition.at(id) >= pos) {
      return;
    }
    m_variablePosition.at(id) = pos;
    tmpMark.at(id) = true;

    for (auto invariant : graph.m_listeningInvariants.at(id)) {
      // for each variable defined by that invariant
      for (auto dependentVariable :
           graph.m_variablesDefinedByInvariant.at(invariant)) {
        if (!tmpMark.at(dependentVariable)) {
          size_t nextPos = pos + ((isPartOfCycle.at(id) &&
                                           isPartOfCycle.at(dependentVariable)
                                       ? 0
                                       : 1));
          visit(dependentVariable, nextPos);
        }
      }
    }
    tmpMark.at(id) = false;
    return;
  };

  for (size_t i = 0; i < graph.m_numVariables + 1; i++) {
    if (i == NULL_ID) {  // Skip nullVar if any.
      continue;
    }
    detectCycle(VarId(i), VarId(i));
  }

  // Call visit on all top variables
  for (size_t i = 0; i < graph.m_numVariables + 1; i++) {
    if (i == NULL_ID) {  // Skip nullVar if any.
      continue;
    }
    if (graph.m_definingInvariant.at(i).id == NULL_ID) {
      visit(VarId(i), 1);
    }
  }
  computeInvariantFromVariables();
}

/**
 * Computes a topology from the current topology of variables.
 * Notes that this gives different key-domains to Variables and invariants.
 */
void TopDownPropagationGraph::Topology::computeInvariantFromVariables() {
  m_invariantPosition.resize(graph.m_numInvariants + 1, 0);
  for (size_t i = 0; i < graph.m_numInvariants + 1; i++) {
    if (i == NULL_ID) {  // Skip nullVar if any.
      continue;
    }
    InvariantId invariant = InvariantId(i);
    size_t position = 0;
    for (auto dependentVariable :
         graph.m_variablesDefinedByInvariant.at(invariant)) {
      position =
          std::max<size_t>(position, m_variablePosition.at(dependentVariable));
    }
    m_invariantPosition.at(invariant) = position;
  }
}
