#pragma once

#include <assert.h>

#include <memory>
#include <queue>
#include <vector>

#include "core/intVar.hpp"
#include "core/invariant.hpp"
#include "core/tracer.hpp"
#include "core/types.hpp"
#include "propagation/propagationGraph.hpp"

class Engine {
 private:
  static const size_t ESTIMATED_NUM_OBJECTS = 1000;

  Timestamp m_currentTime;

  std::vector<std::shared_ptr<IntVar>> m_intVars;
  std::vector<std::shared_ptr<Invariant>> m_invariants;

  PropagationGraph m_propGraph;

 public:
  static const Id NULL_ID;
  Engine(/* args */);
  ~Engine();

  //--------------------- Move semantics ---------------------
  void beginMove(Timestamp& t);
  void endMove(Timestamp& t);

  //--------------------- Notificaion ---------------------
  /***
   * @param t the timestamp when the changed happened
   * @param id the id of the changed variable
   */
  void notifyMaybeChanged(const Timestamp& t, Id id);

  //--------------------- Variable ---------------------
  void incValue(const Timestamp&, VarId&, Int inc);
  void setValue(const Timestamp&, VarId&, Int val);

  Int getValue(const Timestamp&, VarId&);
  Int getCommitedValue(VarId&);

  Timestamp getTimestamp(VarId&);

  void commit(VarId&);  // todo: this feels dangerous, maybe commit should
                        // always have a timestamp?
  void commitIf(const Timestamp&, VarId&);
  void commitValue(const Timestamp&, VarId&, Int val);

  //--------------------- Registration ---------------------
  /**
   * Register an invariant in the engine and return its new id.
   * This also sets the id of the invariant to the new id.
   * @param invariantPtr pointer to the invariant to register
   * @return the id of the invariant in the engine.
   */
  InvariantId registerInvariant(std::shared_ptr<Invariant> invariantPtr);

  /**
   * Creates an IntVar and registers it to the engine.
   * @return the created IntVar
   */
  VarId makeIntVar();

  /**
   * Register that Invariant to depends on variable from depends on dependency
   * @param dependee the invariant that the variable depends on
   * @param source the depending variable
   * @param localId the id of the depending variable in the invariant
   * @param data additioonal data
   */
  void registerInvariantDependsOnVar(InvariantId dependee, VarId source,
                                     LocalId localId, Int data);

  /**
   * Register that 'from' defines variable 'to'. Throws exception if
   * already defined.
   * @param dependee the variable that is defined by the invariant
   * @param source the invariant defining the variable
   * @throw if the variable is already defined by an invariant.
   */
  void registerDefinedVariable(VarId dependee, InvariantId source);
};