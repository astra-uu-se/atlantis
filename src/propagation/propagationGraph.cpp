#include "propagation/propagationGraph.hpp"

#include <algorithm>
#include <iostream>
#include "misc/logging.hpp"

PropagationGraph::PropagationGraph(size_t expectedSize)
    : m_numInvariants(0),
      m_numVariables(0),
      m_definingInvariant(expectedSize),
      m_variablesDefinedByInvariant(expectedSize),
      m_inputVariables(expectedSize),
      m_listeningInvariants(expectedSize),
      m_topology(*this) {
  // m_decisionVariables;
  // m_outputVariables;
}

void PropagationGraph::registerInvariant([[maybe_unused]] InvariantId id) {
  // Everything must be registered in sequence.
  m_variablesDefinedByInvariant.register_idx(id);
  m_inputVariables.register_idx(id);
  ++m_numInvariants;
}

void PropagationGraph::registerVar([[maybe_unused]] VarIdBase id) {
  m_listeningInvariants.register_idx(id);
  m_definingInvariant.register_idx(id);
  ++m_numVariables;
}

void PropagationGraph::registerInvariantDependsOnVar(InvariantId dependent,
                                                     VarIdBase source) {
  assert(!dependent.equals(NULL_ID) && !source.equals(NULL_ID));
  if (m_definingInvariant[source] == dependent) {
    logWarning("The dependent invariant ("
               << dependent << ") already "
               << "defines the source variable (" << source << "); "
               << "ignoring (selft-cyclic) dependency.");
    return;
  }
  m_listeningInvariants[source].push_back(dependent);
  m_inputVariables[dependent].push_back(source);
}

void PropagationGraph::registerDefinedVariable(VarIdBase dependent,
                                               InvariantId source) {
  assert(!dependent.equals(NULL_ID) && !source.equals(NULL_ID));
  if (m_definingInvariant.at(dependent).id != NULL_ID.id) {
    throw VariableAlreadyDefinedException(
        "Variable " + std::to_string(dependent.id) +
        " already defined by invariant " +
        std::to_string(m_definingInvariant.at(dependent).id));
  }
  Int index = -1;
  for (size_t i = 0; i < m_listeningInvariants[dependent].size(); ++i) {
    if (m_listeningInvariants[dependent][i] == source) {
      index = i;
      break;
    }
  }
  if (index >= 0) {
    m_listeningInvariants[dependent].erase(
        m_listeningInvariants[dependent].begin() + index);
    logWarning("The (self-cyclic) dependency that the source invariant "
               << "(" << source << ") depends on the dependent "
               << "variable (" << source << ") was removed.");
  }
  m_definingInvariant[dependent] = source;
  m_variablesDefinedByInvariant[source].push_back(dependent);
}

void PropagationGraph::close() {
  m_isDecisionVar.resize(getNumVariables() + 1);
  m_isOutputVar.resize(getNumVariables() + 1);
  for (size_t i = 1; i < getNumVariables() + 1; ++i) {
    m_isOutputVar.at(i) = (m_listeningInvariants.at(i).empty());
    m_isDecisionVar.at(i) = (m_definingInvariant.at(i) == NULL_ID);
    if (m_isOutputVar.at(i)) {
      m_outputVariables.push_back(i);
    }
    if (m_isDecisionVar.at(i)) {
      m_decisionVariables.push_back(i);
    }
  }

  m_topology.computeLayersWithCycles();
  // Reset propagation queue data structure.
  // TODO: Be sure that this does not cause a memeory leak...
  // m_propagationQueue = PropagationQueue();
  size_t numLayers =
      1 + *std::max_element(m_topology.m_variablePosition.begin(),
                            m_topology.m_variablePosition.end());
  m_propagationQueue.init(getNumVariables(), numLayers);
  for (size_t i = 1; i < getNumVariables() + 1; ++i) {
    VarIdBase id = VarIdBase(i);
    m_propagationQueue.initVar(id, m_topology.getPosition(id));
  }
  //  m_topology.computeNoCycles();
}