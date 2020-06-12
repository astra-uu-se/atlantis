#include "propagation/topDownPropagationGraph.hpp"

TopDownPropagationGraph::TopDownPropagationGraph(size_t expectedSize)
    : PropagationGraph(expectedSize),
      m_modifiedVariables(PriorityCmp(m_topology)) {
  m_varsLastChange.reserve(expectedSize);

  m_varsLastChange.push_back(NULL_TIMESTAMP);
}

void TopDownPropagationGraph::notifyMaybeChanged(const Timestamp& t, VarId id) {
  if (m_varsLastChange.at(id) == t) {
    return;
  }
  m_varsLastChange[id] = t;
  m_modifiedVariables.push(id);
}

void TopDownPropagationGraph::registerVar(VarId id) {
  PropagationGraph::registerVar(id);  // call parent implementation
  assert(id.id == m_varsLastChange.size());
  m_varsLastChange.push_back(NULL_TIMESTAMP);
}