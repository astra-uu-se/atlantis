#include "propagation/topDownPropagationGraph.hpp"

TopDownPropagationGraph::TopDownPropagationGraph(size_t expectedSize)
    : PropagationGraph(expectedSize),
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