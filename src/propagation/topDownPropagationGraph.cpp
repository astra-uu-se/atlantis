#include "propagation/topDownPropagationGraph.hpp"

#include "core/engine.hpp"

TopDownPropagationGraph::TopDownPropagationGraph(size_t expectedSize)
    : PropagationGraph(expectedSize),
      m_modifiedVariables(PriorityCmp(m_topology)) {
  m_propagatedAt.reserve(expectedSize);
  m_varsLastChange.reserve(expectedSize);
  m_varMark.reserve(expectedSize);
  m_invariantMark.reserve(expectedSize);

  m_variablesToExplore.reserve(expectedSize);

  m_tmpVarContainer.reserve(expectedSize);
  m_tmpBoolContainer.reserve(expectedSize);

  m_varsLastChange.push_back(NULL_TIMESTAMP);
  m_propagatedAt.push_back(NULL_TIMESTAMP);
  m_varMark.push_back(NULL_TIMESTAMP);
  m_invariantMark.push_back(NULL_TIMESTAMP);

  m_tmpVarContainer.push_back(NULL_ID);
  m_tmpBoolContainer.push_back(false);
}

void TopDownPropagationGraph::notifyMaybeChanged(const Timestamp& t, VarId id) {
  if (m_varsLastChange.at(id) == t || !isActive(t, id)) {
    return;
  }
  m_varsLastChange.at(id) = t;

  m_modifiedVariables.push(id);
}

bool TopDownPropagationGraph::isActive(const Timestamp& t, VarId id) {
  return m_varMark.at(id) == t;
}

bool TopDownPropagationGraph::isActive(const Timestamp& t, InvariantId id) {
  return m_invariantMark.at(id) == t;
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
  assert(id.id == m_propagatedAt.size());
  m_propagatedAt.push_back(NULL_TIMESTAMP);
  assert(id.id == m_varMark.size());
  m_varMark.push_back(NULL_TIMESTAMP);

  m_tmpVarContainer.push_back(NULL_ID);
  m_tmpBoolContainer.push_back(false);
}

void TopDownPropagationGraph::registerInvariant(InvariantId id) {
  PropagationGraph::registerInvariant(id);
  assert(id.id == m_invariantMark.size());
  m_invariantMark.push_back(NULL_TIMESTAMP);
}

void TopDownPropagationGraph::clearForPropagation() {
  // Clear the queue: we will rebuild it based on registered vars.
  // Usually the queue is very small at this point so this should be cheap.
  while (!m_modifiedVariables.empty()) {
    m_modifiedVariables.pop();
  }

  m_variablesToExplore.clear();
}

void TopDownPropagationGraph::registerForPropagation(const Timestamp& t,
                                                     VarId id) {
  if (m_propagatedAt.at(id) == t) {
    return;
  }
  m_variablesToExplore.push_back(id);
}

/**
 * This method expects something else to update m_propagatedAt.at(nextVar)
 * While propagation is happening.
 */

void TopDownPropagationGraph::schedulePropagation(const Timestamp& t,
                                                  const Engine& e) {
  // If a variable has been visited before (during this call) then do not
  // enqueue it.
  // Note that we cannot use m_varMark for this, as marked variables can be
  // requeued during other calls with the same timestamp.
  std::vector<bool>& visited = m_tmpBoolContainer;
  visited.assign(visited.size(), false);

  while (!m_variablesToExplore.empty()) {
    VarId currentVar = m_variablesToExplore.back();
    m_variablesToExplore.pop_back();
    if (visited.at(currentVar)) {
      continue;  // guessing that it is faster to have multiple occurrences and
                 // skipping, than the filter the insert.
    }
    m_varMark.at(currentVar) = t;  // currentVar is marked for this timestamp.
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
      m_invariantMark.at(definingInvariant) =
          t;  // The defining invariant is also marked.
      std::vector<VarId>& inputVars = m_inputVariables.at(definingInvariant);
      m_variablesToExplore.insert(m_variablesToExplore.end(), inputVars.begin(),
                                  inputVars.end());
    }
  }
}