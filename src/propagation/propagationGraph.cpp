#include "propagation/propagationGraph.hpp"

#include "exceptions/exceptions.hpp"

extern Id NULL_ID;

PropagationGraph::PropagationGraph(size_t expectedSize) {
  m_definingInvariant.reserve(expectedSize);
  m_variablesDefinedByInvariant.reserve(expectedSize);
  m_listeningInvariants.reserve(expectedSize);

  // Initialise nullID
  m_definingInvariant.push_back(InvariantId(NULL_ID));
  m_variablesDefinedByInvariant.push_back({});
  m_listeningInvariants.push_back({});
}

PropagationGraph::~PropagationGraph() {}

void PropagationGraph::notifyMaybeChanged([[maybe_unused]] const Timestamp& t,
                                          VarId id) {
  //TODO: check validity here?
  // If first time variable is invalidated:
  // if (m_intVars.at(id)->m_isInvalid) {
    // m_intVars.at(id)->invalidate(t);
    // m_propGraph.notifyMaybeChanged(t, id);
  // }
  m_modifiedVariables.push(id);
}

void PropagationGraph::registerInvariant(InvariantId id) {
  // Everything must be registered in sequence.
  assert(id.id == m_variablesDefinedByInvariant.size());
  m_variablesDefinedByInvariant.push_back({});
}

void PropagationGraph::registerVar(VarId id) {
  assert(id.id == m_listeningInvariants.size());
  assert(id.id == m_definingInvariant.size());
  m_listeningInvariants.push_back({});
  m_definingInvariant.push_back(InvariantId(NULL_ID));
}

void PropagationGraph::registerInvariantDependsOnVar(InvariantId dependee,
                                                     VarId source,
                                                     LocalId localId,
                                                     Int data) {
  assert(!dependee.equals(NULL_ID) && !source.equals(NULL_ID));
  m_listeningInvariants.at(source).emplace_back(
      InvariantDependencyData{dependee, localId, data});
#ifdef VERBOSE_TRACE
#include <iostream>
  std::cout << "Registering that invariant " << dependee
            << " depends on variable " << source << " with local id " << localId
            << "\n";
#endif
}

void PropagationGraph::registerDefinedVariable(VarId dependee,
                                               InvariantId source) {
  assert(!dependee.equals(NULL_ID) && !source.equals(NULL_ID));
#ifdef VERBOSE_TRACE
#include <iostream>
  std::cout << "Registering that invariant " << source << " defines variable "
            << dependee << "\n";
#endif
  std::cout << m_definingInvariant.size() << " " << dependee << std::endl;
  std::cout << m_variablesDefinedByInvariant.size() << " " << source
            << std::endl;
  if (m_definingInvariant.at(dependee).id == NULL_ID.id) {
    m_definingInvariant.at(dependee) = source;
    m_variablesDefinedByInvariant.at(source).push_back(dependee);
  } else {
    throw new VariableAlreadyDefinedException(
        "Variable " + std::to_string(dependee.id) +
        " already defined by invariant " +
        std::to_string(m_definingInvariant.at(dependee).id));
  }
}
