#include "propagation/propagationGraph.hpp"

PropagationGraph::PropagationGraph(size_t expectedSize)
    : m_numInvariants(0),
      m_numVariables(0),
      m_definingInvariant(expectedSize),
      m_variablesDefinedByInvariant(expectedSize),
      m_inputVariables(expectedSize),
      m_listeningInvariants(expectedSize),
      m_topology(*this) {}

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
  m_listeningInvariants[source].push_back(dependent);
  m_inputVariables[dependent].push_back(source);
}

void PropagationGraph::registerDefinedVariable(VarIdBase dependent,
                                               InvariantId source) {
  assert(!dependent.equals(NULL_ID) && !source.equals(NULL_ID));
  if (m_definingInvariant.at(dependent).id == NULL_ID.id) {
    m_definingInvariant[dependent] = source;
    m_variablesDefinedByInvariant[source].push_back(dependent);
  } else {
    throw VariableAlreadyDefinedException(
        "Variable " + std::to_string(dependent.id) +
        " already defined by invariant " +
        std::to_string(m_definingInvariant.at(dependent).id));
  }
}

void PropagationGraph::close() {
  m_isInputVar.resize(getNumVariables() + 1);
  m_isOutputVar.resize(getNumVariables() + 1);
  for (size_t i = 1; i < getNumVariables() + 1; i++) {
    m_isOutputVar.at(i) = (m_listeningInvariants.at(i).empty());
    m_isInputVar.at(i) = (m_definingInvariant.at(i) == NULL_ID);
  }

  m_topology.computeWithCycles();
  //  m_topology.computeNoCycles();
}
