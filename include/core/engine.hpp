#pragma once

#include <assert.h>

#include <memory>
#include <queue>
#include <vector>

#include "core/intVar.hpp"
#include "core/invariant.hpp"
#include "core/types.hpp"

class Engine {
 private:
  static const size_t ESTIMATED_NUM_OBJECTS = 1000;

  std::vector<std::shared_ptr<IntVar>> m_intVars;
  std::vector<std::shared_ptr<Invariant>> m_invariants;

  /**
   * Map from VarID -> InvariantId
   *
   * Maps to nullptr if not defined by any invariant.
   */
  std::vector<InvariantId> m_definingInvariant;

  /**
   * Map from InvariantId -> list of VarID
   *
   * Maps an invariant to all variables it defines.
   */
  std::vector<std::vector<VarId>> m_variablesDefinedByInvariant;

  struct InvariantDependencyData {
    InvariantId id;
    LocalId localId;
    Int data;
  };
  // Map from VarID -> vector of InvariantID
  std::vector<std::vector<InvariantDependencyData>> m_listeningInvariants;

  std::queue<VarId> m_modifiedVariables;

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
   * This also sets the id of the variable to the new id.
   */
  VarId registerIntVar(std::shared_ptr<IntVar>);
  void registerIntVar(std::vector<std::shared_ptr<IntVar>>);

  /**
   * Register that target depends on dependency
   */
  void registerInvariantDependency(InvariantId to, VarId from, LocalId localId,
                                   Int data);

  /**
   * Register that 'from' defines variable 'to'. Throws exception if
   * already defined.
   */
  void registerDefinedVariable(InvariantId from, VarId to);
};