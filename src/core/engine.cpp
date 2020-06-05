#include "core/engine.hpp"

#include "exceptions/exceptions.hpp"

const Id Engine::NULL_ID = Id(0);
Engine::Engine(/* args */)
    : m_intVars(),
      m_invariants(),
      m_definingInvariant(),
      m_variablesDefinedByInvariant(),
      m_listeningInvariants(),
      m_modifiedVariables() {
  m_intVars.reserve(ESTIMATED_NUM_OBJECTS);
  m_invariants.reserve(ESTIMATED_NUM_OBJECTS);
  m_definingInvariant.reserve(ESTIMATED_NUM_OBJECTS);
  m_variablesDefinedByInvariant.reserve(ESTIMATED_NUM_OBJECTS);
  m_listeningInvariants.reserve(ESTIMATED_NUM_OBJECTS);

  // Vectors indexed by IDs are initialised to size 1 so that the nullID is its
  // only initial member.
  m_intVars.push_back(nullptr);     // expands with registerIntVar
  m_invariants.push_back(nullptr);  // expands with registerInvariant
  m_definingInvariant.push_back(
      InvariantId(NULL_ID));  // expands with registerInvariant
  m_variablesDefinedByInvariant.push_back(
      {});                              // expands with registerInvariant
  m_listeningInvariants.push_back({});  // expands with registerIntVar
}

Engine::~Engine() {}

//---------------------Notificaion---------------------
void Engine::notifyMaybeChanged(Int id) {
  // If first time variable is invalidated:
  if (m_intVars.at(id)->m_isInvalid) {
    m_intVars.at(id)->invalidate();
    m_modifiedVariables.push(id);
  }
}

//---------------------Registration---------------------

InvariantId Engine::registerInvariant(std::shared_ptr<Invariant> invariantPtr) {
  m_invariants.push_back(invariantPtr);
  m_variablesDefinedByInvariant.push_back({});
  assert(m_invariants.size() == m_variablesDefinedByInvariant.size());
  InvariantId newId = InvariantId(m_invariants.size() - 1);
  invariantPtr->setId(newId);
#ifdef VERBOSE_TRACE
#include <iostream>
  std::cout << "Registering new invariant with id: " << newId << "\n";
#endif
  return newId;
}

VarId Engine::registerIntVar(std::shared_ptr<IntVar> intVarPtr) {
  m_intVars.push_back(intVarPtr);
  m_listeningInvariants.push_back({});
  m_definingInvariant.push_back(InvariantId(NULL_ID));
  assert(m_intVars.size() == m_listeningInvariants.size());
  assert(m_intVars.size() == m_definingInvariant.size());
  VarId newId = VarId(m_intVars.size() - 1);
  intVarPtr->setId(newId);
#ifdef VERBOSE_TRACE
#include <iostream>
  std::cout << "Registering new variable with id: " << newId << "\n";
#endif
  return newId;
}

std::shared_ptr<IntVar> Engine::makeIntVar() {
  VarId newId = VarId(m_intVars.size());

  m_intVars.emplace_back(std::make_shared<IntVar>(newId));
  m_listeningInvariants.push_back({});
  m_definingInvariant.push_back(InvariantId(NULL_ID));
  assert(m_intVars.size() == m_listeningInvariants.size());
  assert(m_intVars.size() == m_definingInvariant.size());
  return m_intVars.back();
}

void Engine::registerIntVar(std::vector<std::shared_ptr<IntVar>> intVarPtrs) {
  for (auto p : intVarPtrs) {
    registerIntVar(p);
  }
}

void Engine::registerInvariantDependency(InvariantId to, VarId from,
                                         LocalId localId, Int data) {
  assert(!to.equals(NULL_ID) && !from.equals(NULL_ID));
  m_listeningInvariants.at(from).emplace_back(
      InvariantDependencyData{to, localId, data});
  m_variablesDefinedByInvariant.push_back({});
#ifdef VERBOSE_TRACE
#include <iostream>
  std::cout << "Registering that invariant " << to << " depends on variable "
            << from << " with local id " << localId << "\n";
#endif
}

void Engine::registerDefinedVariable(InvariantId from, VarId to) {
  assert(!to.equals(NULL_ID) && !from.equals(NULL_ID));
#ifdef VERBOSE_TRACE
#include <iostream>
  std::cout << "Registering that invariant " << from << " defines variable "
            << to << "\n";
#endif
  if (m_definingInvariant.at(to).id == NULL_ID.id) {
    m_definingInvariant.at(to) = from;
    m_variablesDefinedByInvariant.at(from).push_back(to);
  } else {
    throw new VariableAlreadyDefinedException(
        "Variable " + std::to_string(to.id) + " already defined by invariant " +
        std::to_string(m_definingInvariant.at(to).id));
  }
}