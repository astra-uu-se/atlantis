#include "core/engine.hpp"
#include "exceptions/exceptions.hpp"

const Id Engine::NULL_ID = Id(0);
Engine::Engine(/* args */)
    : m_intVars(0),
      m_invariants(0),
      m_definingInvariant(),
      m_listeningInvariants() {
  m_intVars.reserve(ESTIMATED_NUM_OBJECTS);
  m_invariants.reserve(ESTIMATED_NUM_OBJECTS);
  m_definingInvariant.reserve(ESTIMATED_NUM_OBJECTS);
  m_listeningInvariants.reserve(ESTIMATED_NUM_OBJECTS);
  // Vectors indexed by IDs are initialised to size 1 so that the nullID is its
  // only initial member.
  m_intVars.push_back(nullptr);
  m_invariants.push_back(nullptr);
  m_definingInvariant.push_back(InvariantId(NULL_ID));
  m_listeningInvariants.push_back({});
}

Engine::~Engine() {}

InvariantId Engine::registerInvariant(std::shared_ptr<Invariant> invariantPtr) {
  m_invariants.push_back(invariantPtr);
  m_definingInvariant.push_back(InvariantId(NULL_ID));
  assert(m_invariants.size() == m_definingInvariant.size());
  InvariantId newId = InvariantId(m_intVars.size() - 1);
  invariantPtr->setId(newId);
  return newId;
}

VarId Engine::registerIntVar(std::shared_ptr<IntVar> intVarPtr) {
  m_intVars.push_back(intVarPtr);
  m_listeningInvariants.push_back({});
  assert(m_intVars.size() == m_listeningInvariants.size());
  VarId newId = VarId(m_intVars.size() - 1);
  intVarPtr->setId(newId);
  return newId;
}

void Engine::registerInvariantDependency(InvariantId to, VarId from,
                                         LocalId localId, Int data) {
  m_listeningInvariants.at(from).emplace_back(
      InvariantDependencyData{to, localId, data});
}

void Engine::registerDefinedVariable(InvariantId from, VarId to) {
  if (m_definingInvariant[to].id == NULL_ID.id) {
    m_definingInvariant[to] = from;
  } else {
    throw new VariableAlreadyDefinedException(
        "Variable " + std::to_string(to.id) + " already defined by invariant " +
        std::to_string(m_definingInvariant[to].id));
  }
}