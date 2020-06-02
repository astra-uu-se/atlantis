#pragma once
#include "core/invariant.hpp"
#include "core/types.hpp"

class Engine {
 private:
 public:
  static const Id NULL_ID;
  Engine(/* args */);
  ~Engine();

  //---------------------Notificaion
  void notifyMaybeChanged(Int id);

  //---------------------Registration
  /**
   * Register an invariant in the engine and return its new id.
   * TODO: this should also set the id of the invariant to the new id.
   */
  InvariantId registerInvariant(Invariant& i);

  /**
   * Register that target depends on dependency
   */
  void registerDependency(InvariantId target, VarId dependency, LocalId localId, Int data);

  /**
   * Register that source defines variable definedVar. Throws exception if
   * already defined.
   */
  void registerDefinedVariable(InvariantId source, VarId definedVar);
};