#pragma once

#include <assert.h>

#include <memory>
#include <vector>

#include "core/intVar.hpp"
#include "core/invariant.hpp"
#include "core/types.hpp"

class Engine {
 private:
  static const size_t ESTIMATED_NUM_OBJECTS = 1000;

  std::vector<std::shared_ptr<IntVar>> m_intVars;
  std::vector<std::shared_ptr<Invariant>> m_invariants;

  // Map from InvariantID -> vector of VarID
  std::vector<std::vector<VarId>> m_dependentVariables;
  // Map from VarID -> vector of InvariantID
  std::vector<std::vector<InvariantId>> m_dependentInvariants;

 public:
  static const Id NULL_ID;
  Engine(/* args */);
  ~Engine();

  //---------------------Notificaion
  void notifyMaybeChanged(Int id);

  //---------------------Registration
  /**
   * Register an invariant in the engine and return its new id.
   * This also sets the id of the invariant to the new id.
   */
  InvariantId registerInvariant(std::shared_ptr<Invariant>);

  /**
   * Register an intVar in the engine and return its new id.
   * This also sets the id of the invariant to the new id.
   */
  VarId registerIntVar(std::shared_ptr<IntVar>);

  /**
   * Register that target depends on dependency
   */
  void registerDependency(InvariantId target, VarId dependency, LocalId localId,
                          Int data);

  /**
   * Register that source defines variable definedVar. Throws exception if
   * already defined.
   */
  void registerDefinedVariable(InvariantId source, VarId definedVar);
};