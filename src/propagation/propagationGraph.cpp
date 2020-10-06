#include "propagation/propagationGraph.hpp"

#include <algorithm>
#include <functional>

#include "exceptions/exceptions.hpp"
#include "topology.cpp"
extern Id NULL_ID;

PropagationGraph::PropagationGraph(size_t expectedSize)
    : m_numInvariants(0), m_numVariables(0), m_topology(*this) {
  m_definingInvariant.reserve(expectedSize);
  m_variablesDefinedByInvariant.reserve(expectedSize);
  m_inputVariables.reserve(expectedSize);
  m_listeningInvariants.reserve(expectedSize);

  // Initialise nullID
  m_definingInvariant.push_back(InvariantId(NULL_ID));
  m_variablesDefinedByInvariant.push_back({});
  m_inputVariables.push_back({});
  m_listeningInvariants.push_back({});
}

void PropagationGraph::registerInvariant([[maybe_unused]] InvariantId id) {
  // Everything must be registered in sequence.
  assert(id.id == m_variablesDefinedByInvariant.size());
  m_variablesDefinedByInvariant.push_back({});
  assert(id.id == m_inputVariables.size());
  m_inputVariables.push_back({});
  ++m_numInvariants;
}

void PropagationGraph::registerVar([[maybe_unused]] VarId id) {
  assert(id.id == m_listeningInvariants.size());
  assert(id.id == m_definingInvariant.size());
  m_listeningInvariants.push_back({});
  m_definingInvariant.push_back(InvariantId(NULL_ID));
  ++m_numVariables;
}

void PropagationGraph::registerInvariantDependsOnVar(InvariantId dependent,
                                                     VarId source) {
  assert(!dependent.equals(NULL_ID) && !source.equals(NULL_ID));
  m_listeningInvariants.at(source).push_back(dependent);
  m_inputVariables.at(dependent).push_back(source);
}

void PropagationGraph::registerDefinedVariable(VarId dependent,
                                               InvariantId source) {
  assert(!dependent.equals(NULL_ID) && !source.equals(NULL_ID));
  if (m_definingInvariant.at(dependent).id == NULL_ID.id) {
    m_definingInvariant.at(dependent) = source;
    m_variablesDefinedByInvariant.at(source).push_back(dependent);
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
  for (size_t i = 0; i < getNumVariables() + 1; i++) {
    m_isOutputVar.at(i) = (m_listeningInvariants.at(i).size() == 0);
    m_isInputVar.at(i) = (m_definingInvariant.at(i) == NULL_ID);
  }

  m_topology.computeWithCycles();
//  m_topology.computeNoCycles();
}
