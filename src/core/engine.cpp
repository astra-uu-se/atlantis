#include "core/engine.hpp"

const Id Engine::NULL_ID = Id(0);
Engine::Engine(/* args */)
    : m_intVars(0),
      m_invariants(0),
      m_dependentVariables(0),
      m_dependentInvariants(0) {
  m_intVars.reserve(ESTIMATED_NUM_OBJECTS);
  m_invariants.reserve(ESTIMATED_NUM_OBJECTS);
  m_dependentVariables.reserve(ESTIMATED_NUM_OBJECTS);
  m_dependentInvariants.reserve(ESTIMATED_NUM_OBJECTS);
  // Vectors indexed by IDs are initialised to size 1 so that the nullID is its
  // only initial member.
  m_intVars.push_back(nullptr);
  m_invariants.push_back(nullptr);
  m_dependentVariables.push_back({});
  m_dependentInvariants.push_back({});
}

Engine::~Engine() {}

InvariantId Engine::registerInvariant(std::shared_ptr<Invariant> invariantPtr) {
  m_invariants.push_back(invariantPtr);
  m_dependentVariables.push_back(std::vector<VarId>());
  assert(m_invariants.size() == m_dependentVariables.size());
  InvariantId newId = InvariantId(m_intVars.size() - 1);
  invariantPtr->setId(newId);
  return newId;
}

VarId Engine::registerIntVar(std::shared_ptr<IntVar> intVarPtr) {
  m_intVars.push_back(intVarPtr);
  m_dependentInvariants.push_back({});
  assert(m_intVars.size() == m_dependentInvariants.size());
  VarId newId = VarId(m_intVars.size() - 1);
  intVarPtr->setId(newId);
  return newId;
}

/**
 * Register that target depends on dependency
 */
void Engine::registerInvariantDependency(InvariantId target, VarId dependency,
                                         LocalId localId, Int data) {
  m_dependentInvariants.at(dependency)
      .emplace_back(InvariantDependencyData{target, localId, data});
}

/**
 * Register that source defines variable definedVar. Throws exception if
 * already defined.
 */
void Engine::registerDefinedVariable(InvariantId source, VarId definedVar) {}