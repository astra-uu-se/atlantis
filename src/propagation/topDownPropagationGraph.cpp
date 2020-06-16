#include "propagation/topDownPropagationGraph.hpp"

#include "core/engine.hpp"

TopDownPropagationGraph::TopDownPropagationGraph(size_t expectedSize)
    : PropagationGraph(expectedSize),
      m_modifiedVariables(PriorityCmp(m_topology)) {

  m_propagatedAt.reserve(expectedSize);
  m_varsLastChange.reserve(expectedSize);
  m_tmpVarContainer.reserve(expectedSize);
  m_tmpMarkContainer.reserve(expectedSize);

  m_varsLastChange.push_back(NULL_TIMESTAMP);
  m_propagatedAt.push_back(NULL_TIMESTAMP);

  m_tmpVarContainer.push_back(NULL_ID);
  m_tmpMarkContainer.push_back(false);
}

void TopDownPropagationGraph::notifyMaybeChanged(const Timestamp& t, VarId id) {
  if (m_varsLastChange.at(id) == t || !isActive(t, id)) {
    return;
  }
  m_varsLastChange.at(id) = t;

  m_modifiedVariables.push(id);
}

VarId TopDownPropagationGraph::getNextStableVariable(const Timestamp& t) {
  if (m_modifiedVariables.empty()) {
    return VarId(NULL_ID);
  }
  VarId nextVar = m_modifiedVariables.top();
  m_modifiedVariables.pop();
  m_propagatedAt.at(nextVar) = t;
  // Due to notifyMaybeChanged, all variables in the queue are "active".
  return nextVar;
}

void TopDownPropagationGraph::registerVar(VarId id) {
  PropagationGraph::registerVar(id);  // call parent implementation
  assert(id.id == m_varsLastChange.size());
  m_varsLastChange.push_back(NULL_TIMESTAMP);
  m_propagatedAt.push_back(NULL_TIMESTAMP);

  m_tmpVarContainer.push_back(NULL_ID);
  m_tmpMarkContainer.push_back(false);
}

// void TopDownPropagationGraph::registerInvariant(InvariantId id) {
//   PropagationGraph::registerInvariant(id);
// }

/**
 * This method expects something else to update m_propagatedAt.at(nextVar)
 * While propagation is happening.
 */

void TopDownPropagationGraph::schedulePropagationFor(const Timestamp& t,
                                                     const Engine& e,
                                                     VarId targetVar) {
  if (m_propagatedAt.at(targetVar) == t) {
    return;  // noting to do.
  }

  // Clear the queue: we will rebuild it based on targetVar.
  // Usually the queue is very small at this point so this should be cheap.
  while (!m_modifiedVariables.empty()) {
    m_modifiedVariables.pop();
  }
  // If a variable has been visited before (during this call) then do not
  // enqueue it.
  std::vector<bool>& visited = m_tmpMarkContainer;
  visited.assign(visited.size(), false);

  std::vector<VarId>& variablesToExplore = m_tmpVarContainer;
  variablesToExplore.clear();  // clears vector but keeps the allocated size.

  variablesToExplore.push_back(targetVar);

  while (!variablesToExplore.empty()) {
    VarId currentVar = variablesToExplore.back();
    variablesToExplore.pop_back();
    if (visited.at(currentVar)) {
      continue;  // guessing that it is faster to have multiple occurrences and
                 // skipping, than the filter the insert.
    }
    visited.at(currentVar) = true;
    // If the variable has changed, then enqueue it.
    if (e.hasChanged(t, currentVar)) {
      m_modifiedVariables.push(currentVar);
    }
    // If the variable was propagated at this timestep, then all of its parent
    // nodes have already propagated and there is not need to explore them.
    if (m_propagatedAt.at(currentVar) != t &&
        m_definingInvariant.at(currentVar) != NULL_ID) {
      InvariantId definingInvariant = m_definingInvariant.at(currentVar);
      std::vector<VarId>& inputVars = m_inputVariables.at(definingInvariant);
      variablesToExplore.insert(variablesToExplore.end(), inputVars.begin(),
                                inputVars.end());
    }
  }
}