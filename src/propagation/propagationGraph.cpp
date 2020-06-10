#include "propagation/propagationGraph.hpp"

#include <algorithm>

#include "exceptions/exceptions.hpp"
extern Id NULL_ID;

PropagationGraph::PropagationGraph(size_t expectedSize)
    : m_numInvariants(0), m_numVariables(0), m_topology(*this) {
  m_definingInvariant.reserve(expectedSize);
  m_variablesDefinedByInvariant.reserve(expectedSize);
  m_listeningInvariants.reserve(expectedSize);
  m_varsLastCommit.reserve(expectedSize);

  // Initialise nullID
  m_definingInvariant.push_back(InvariantId(NULL_ID));
  m_variablesDefinedByInvariant.push_back({});
  m_listeningInvariants.push_back({});
  m_varsLastCommit.push_back(NULL_TIMESTAMP);
}

void PropagationGraph::notifyMaybeChanged([[maybe_unused]] const Timestamp& t,
                                          VarId id) {
  // TODO: check validity here?
  // If first time variable is invalidated:
  // if (m_intVars.at(id)->m_isInvalid) {
  // m_intVars.at(id)->invalidate(t);
  // m_propGraph.notifyMaybeChanged(t, id);
  // }
  if (m_varsLastCommit.at(id) == t) {
    return;
  }
  m_varsLastCommit[id] = t;
  m_modifiedVariables.push(id);
}

void PropagationGraph::registerInvariant(InvariantId id) {
  // Everything must be registered in sequence.
  assert(id.id == m_variablesDefinedByInvariant.size());
  m_variablesDefinedByInvariant.push_back({});
  ++m_numInvariants;
}

void PropagationGraph::registerVar(VarId id) {
  assert(id.id == m_listeningInvariants.size());
  assert(id.id == m_definingInvariant.size());
  assert(id.id == m_varsLastCommit.size());
  m_listeningInvariants.push_back({});
  m_definingInvariant.push_back(InvariantId(NULL_ID));
  m_varsLastCommit.push_back(NULL_TIMESTAMP);
  ++m_numVariables;
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

void PropagationGraph::Topology::computeTopology() {
  m_variablePosition.resize(graph.m_numVariables+1, 0);
  m_invariantPosition.resize(graph.m_numInvariants+1, 0);

  // We only keep track of a variable frontier. Invariants are visited on the
  // fly.
  std::queue<VarId> frontier;

  // Find the top level variables that are not defined by anything.
  // todo: it would be cleaning if we maintained a list of all added varIDs but
  // it is more efficient just to recreated based on the total number.
  for (size_t i = 0; i < graph.m_numVariables; i++) {
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
