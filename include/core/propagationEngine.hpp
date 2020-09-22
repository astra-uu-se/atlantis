#pragma once

#include "core/engine.hpp"

class PropagationEngine : public Engine {
 protected:
  BottomUpPropagationGraph m_propGraph;

  void recomputeAndCommit();

  void propagate();
  void bottomUpPropagate();

 public:
  PropagationEngine(/* args */);

  virtual void open() override;
  virtual void close() override;

  //--------------------- Notificaion ---------------------
  /***
   * @param t the timestamp when the changed happened
   * @param id the id of the changed variable
   */
  void notifyMaybeChanged(Timestamp t, VarId id);

  //--------------------- Move semantics ---------------------
  void beginMove();
  void endMove();
  void setValue(Timestamp, VarId, Int val);
  inline void setValue(VarId v, Int val) { setValue(m_currentTime, v, val); }

  void beginQuery();
  void endQuery();
  void query(VarId);

  void beginCommit();
  void endCommit();

  /**
   * returns the next dependency at the current timestamp.
   */
  VarId getNextDependency(InvariantId);

  /**
   * Notify an invariant that its current dependency has changed
   */
  void notifyCurrentDependencyChanged(InvariantId);

  /**
   * Register that Invariant to depends on variable from depends on dependency
   * @param dependent the invariant that the variable depends on
   * @param source the depending variable
   * @param localId the id of the depending variable in the invariant
   * @param data additional data
   */
  virtual void registerInvariantDependsOnVar(InvariantId dependent,
                                             VarId source,
                                             LocalId localId) override;

  /**
   * Register that 'from' defines variable 'to'. Throws exception if
   * already defined.
   * @param dependent the variable that is defined by the invariant
   * @param source the invariant defining the variable
   * @throw if the variable is already defined by an invariant.
   */
  virtual void registerDefinedVariable(VarId dependent,
                                       InvariantId source) override;

  virtual void registerVar(VarId) override;
  virtual void registerInvariant(InvariantId) override;

  BottomUpPropagationGraph& getPropGraph();
};

inline VarId PropagationEngine::getNextDependency(InvariantId inv) {
  return m_store.getInvariant(inv).getNextDependency(m_currentTime, *this);
}
inline void PropagationEngine::notifyCurrentDependencyChanged(InvariantId inv) {
  m_store.getInvariant(inv).notifyCurrentDependencyChanged(m_currentTime,
                                                           *this);
}

inline void PropagationEngine::setValue(Timestamp t, VarId v, Int val) {
  m_store.getIntVar(v).setValue(t, val);
}