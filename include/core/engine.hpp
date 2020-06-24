#pragma once

#include <assert.h>

#include <memory>
#include <vector>

#include "core/constraint.hpp"
#include "core/intVar.hpp"
#include "core/invariant.hpp"
#include "core/tracer.hpp"
#include "core/types.hpp"
#include "exceptions/exceptions.hpp"
#include "propagation/bottomUpPropagationGraph.hpp"
#include "propagation/propagationGraph.hpp"
#include "store/store.hpp"

class Engine {
 private:
  static const size_t ESTIMATED_NUM_OBJECTS = 1000;

  Timestamp m_currentTime;

  BottomUpPropagationGraph m_propGraph;

  bool m_isOpen = true;

  // I don't think dependency data should be part of the store but rather just
  // of the engine.
  struct InvariantDependencyData {
    InvariantId id;
    LocalId localId;
    Int data;
    Timestamp lastNotification;  // todo: unclear if this information is
                                 // relevant for all types of propagation. If
                                 // not then move it to a subclass...
  };
  // Map from VarID -> vector of InvariantID
  std::vector<std::vector<InvariantDependencyData>> m_dependentInvariantData;

  Store m_store;

  void recomputeAndCommit();

  void propagate();
  void bottomUpPropagate();

 public:
  Engine(/* args */);

  void open();
  void close();

  //--------------------- Notificaion ---------------------
  /***
   * @param t the timestamp when the changed happened
   * @param id the id of the changed variable
   */
  void notifyMaybeChanged(Timestamp t, VarId id);

  //--------------------- Move semantics ---------------------
  void beginMove();
  void endMove();

  void beginQuery();
  void endQuery();
  void query(VarId);

  //--------------------- Variable ---------------------
  void incValue(Timestamp, VarId&, Int inc);
  inline void incValue(VarId& v, Int val) { incValue(m_currentTime, v, val); }

  void setValue(Timestamp, VarId&, Int val);
  inline void setValue(VarId& v, Int val) { setValue(m_currentTime, v, val); }
  Int getValue(Timestamp, VarId&);
  inline Int getValue(VarId& v) { return getValue(m_currentTime, v); }

  Int getCommitedValue(VarId&);

  Timestamp getTmpTimestamp(VarId&);

  inline bool hasChanged(Timestamp t, VarId& v) const {
    return m_store.getConstIntVar(v).hasChanged(t);
  }
  void commit(VarId&);  // todo: this feels dangerous, maybe commit should
                        // always have a timestamp?
  void commitIf(Timestamp, VarId&);
  void commitValue(VarId&, Int val);

  /**
   * returns the next depenency at the current timestamp.
   */
  VarId getNextDependency(InvariantId);

  /**
   * Notify an invariant that its current dependency has changed
   */
  void notifyCurrentDependencyChanged(InvariantId);

  //--------------------- Registration ---------------------
  /**
   * Register an invariant in the engine and return its pointer.
   * This also sets the id of the invariant to the new id.
   * @param args the constructor arguments of the invariant
   * @return the created invariant.
   */
  template <class T, typename... Args>
  std::enable_if_t<std::is_base_of<Invariant, T>::value, std::shared_ptr<T>>
  makeInvariant(Args&&... args);

  /**
   * Register a constraint in the engine and return its pointer.
   * This also sets the id of the constraint to the new id.
   * @param args the constructor arguments of the constraint
   * @return the created constraint.
   */
  template <class T, typename... Args>
  std::enable_if_t<std::is_base_of<Constraint, T>::value, std::shared_ptr<T>>
  makeConstraint(Args&&... args);

  /**
   * Creates an IntVar and registers it to the engine.
   * @return the created IntVar
   */
  VarId makeIntVar(Int initValue, Int lowerBound, Int upperBound);

  /**
   * Register that Invariant to depends on variable from depends on dependency
   * @param dependent the invariant that the variable depends on
   * @param source the depending variable
   * @param localId the id of the depending variable in the invariant
   * @param data additional data
   */
  void registerInvariantDependsOnVar(InvariantId dependent, VarId source,
                                     LocalId localId, Int data);

  /**
   * Register that 'from' defines variable 'to'. Throws exception if
   * already defined.
   * @param dependent the variable that is defined by the invariant
   * @param source the invariant defining the variable
   * @throw if the variable is already defined by an invariant.
   */
  void registerDefinedVariable(VarId dependent, InvariantId source);

  const Store& getStore();
  Timestamp getCurrentTime();

  BottomUpPropagationGraph& getPropGraph();
};

template <class T, typename... Args>
std::enable_if_t<std::is_base_of<Invariant, T>::value, std::shared_ptr<T>>
Engine::makeInvariant(Args&&... args) {
  if (!m_isOpen) {
    throw ModelNotOpenException("Cannot make invariant when store is closed.");
  }
  auto invariantPtr = std::make_shared<T>(std::forward<Args>(args)...);

  auto newId = m_store.createInvariantFromPtr(invariantPtr);
  m_propGraph.registerInvariant(newId);
  invariantPtr->init(m_currentTime, *this);
  return invariantPtr;
}

template <class T, typename... Args>
std::enable_if_t<std::is_base_of<Constraint, T>::value, std::shared_ptr<T>>
Engine::makeConstraint(Args&&... args) {
  if (!m_isOpen) {
    throw ModelNotOpenException("Cannot make invariant when store is closed.");
  }
  auto constraintPtr = std::make_shared<T>(std::forward<Args>(args)...);

  auto newId = m_store.createInvariantFromPtr(constraintPtr);
  m_propGraph.registerInvariant(newId);
  constraintPtr->init(m_currentTime, *this);
  return constraintPtr;
}

//--------------------- Inlined functions ---------------------

inline const Store& Engine::getStore() { return m_store; }
inline Timestamp Engine::getCurrentTime() { return m_currentTime; }
inline BottomUpPropagationGraph& Engine::getPropGraph() { return m_propGraph; }

inline Int Engine::getValue(Timestamp t, VarId& v) {
  return m_store.getIntVar(v).getValue(t);
}

inline Int Engine::getCommitedValue(VarId& v) {
  return m_store.getIntVar(v).getCommittedValue();
}

inline Timestamp Engine::getTmpTimestamp(VarId& v) {
  return m_store.getIntVar(v).getTmpTimestamp();
}

inline void Engine::setValue(Timestamp t, VarId& v, Int val) {
  m_store.getIntVar(v).setValue(t, val);
  notifyMaybeChanged(t, v);
}

inline void Engine::incValue(Timestamp t, VarId& v, Int inc) {
  m_store.getIntVar(v).incValue(t, inc);
  notifyMaybeChanged(t, v);
}

inline VarId Engine::getNextDependency(InvariantId inv) {
  return m_store.getInvariant(inv).getNextDependency(m_currentTime, *this);
}
inline void Engine::notifyCurrentDependencyChanged(InvariantId inv){
  m_store.getInvariant(inv).notifyCurrentDependencyChanged(m_currentTime, *this);
}