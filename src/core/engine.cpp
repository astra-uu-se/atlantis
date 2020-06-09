#include "core/engine.hpp"

#include "exceptions/exceptions.hpp"

const Id Engine::NULL_ID = Id(0);
Engine::Engine(/* args */) : m_currentTime(0) {
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

//---------------------Notificaion/Modification---------------------
void Engine::notifyMaybeChanged([[maybe_unused]] const Timestamp& t, Id id) {
  // If first time variable is invalidated:
  if (m_intVars.at(id)->m_isInvalid) {
    m_intVars.at(id)->invalidate(t);
    m_modifiedVariables.push(id);
  }
}

void Engine::setValue(const Timestamp& t, IntVar& v, Int val) {
  v.setValue(t, val);
  notifyMaybeChanged(t, v.m_id);
}

void Engine::incValue(const Timestamp& t, IntVar& v, Int inc) {
  v.incValue(t, inc);
  notifyMaybeChanged(t, v.m_id);
}

void Engine::commit(IntVar& v) {
  v.commit();
  // todo: do something else? like:
  // v.validate();
}

void Engine::commitIf(const Timestamp& t, IntVar& v) {
  v.commitIf(t);
  // todo: do something else? like:
  // v.validate();
}

void Engine::commitValue([[maybe_unused]] const Timestamp& t, IntVar& v, Int val) {
  v.commitValue(val);
  // todo: do something else? like:
  // v.validate();
}

//---------------------Registration---------------------

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
  m_listeningInvariants.push_back(
      {});  // list of invariants that this variable is input to.
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

void Engine::registerInvariantDependency(InvariantId dependee, VarId source,
                                         LocalId localId, Int data) {
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

void Engine::registerDefinedVariable(VarId dependee, InvariantId source) {
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