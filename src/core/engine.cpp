#include "core/engine.hpp"

#include "exceptions/exceptions.hpp"

const Id Engine::NULL_ID = Id(0);
Engine::Engine(/* args */)
    : m_intVars(),
      m_invariants(),
      m_definingInvariant(),
      m_definedVariablesByInvariant(),
      m_listeningInvariants(),
      m_modifiedVariables() {
  m_intVars.reserve(ESTIMATED_NUM_OBJECTS);
  m_invariants.reserve(ESTIMATED_NUM_OBJECTS);
  m_definingInvariant.reserve(ESTIMATED_NUM_OBJECTS);
  m_definedVariablesByInvariant.reserve(ESTIMATED_NUM_OBJECTS);
  m_listeningInvariants.reserve(ESTIMATED_NUM_OBJECTS);

  // Vectors indexed by IDs are initialised to size 1 so that the nullID is its
  // only initial member.
  m_intVars.push_back(nullptr);     // expands with registerIntVar
  m_invariants.push_back(nullptr);  // expands with registerInvariant
  m_definingInvariant.push_back(
      InvariantId(NULL_ID));  // expands with registerInvariant
  m_definedVariablesByInvariant.push_back(
      {});                              // expands with registerInvariant
  m_listeningInvariants.push_back({});  // expands with registerIntVar
}

Engine::~Engine() {}

//---------------------Notificaion---------------------
void Engine::notifyMaybeChanged(Int id) {
  // If first time variable is invalidated:
  if (m_intVars[id]->m_isInvalid) {
    m_intVars[id]->invalidate();
    m_modifiedVariables.push(id);
  }
}

//---------------------Registration---------------------

InvariantId Engine::registerInvariant(std::shared_ptr<Invariant> invariantPtr) {
  m_invariants.push_back(invariantPtr);
  m_definingInvariant.push_back(InvariantId(NULL_ID));
  m_definedVariablesByInvariant.push_back({});
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

void Engine::registerIntVar(std::vector<std::shared_ptr<IntVar>> intVarPtrs) {
  for(auto p: intVarPtrs){
    registerIntVar(p);
  }
}

void Engine::registerInvariantDependency(InvariantId to, VarId from,
                                         LocalId localId, Int data) {
  assert(!to.equals(NULL_ID) && !from.equals(NULL_ID));
  m_listeningInvariants.at(from).emplace_back(
      InvariantDependencyData{to, localId, data});
}

void Engine::registerDefinedVariable(InvariantId from, VarId to) {
  assert(!to.equals(NULL_ID) && !from.equals(NULL_ID));
  if (m_definingInvariant[to].id == NULL_ID.id) {
    m_definingInvariant[to] = from;
    m_definedVariablesByInvariant[from].push_back(to);
  } else {
    throw new VariableAlreadyDefinedException(
        "Variable " + std::to_string(to.id) + " already defined by invariant " +
        std::to_string(m_definingInvariant[to].id));
  }
}