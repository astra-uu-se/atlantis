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
  void notifyMaybeChanged(const Timestamp& t, VarId id);

  //--------------------- Move semantics ---------------------
  void beginMove();
  void endMove();

  void beginQuery();
  void endQuery();
  void query(VarId);

  //--------------------- Variable ---------------------
  void incValue(const Timestamp&, VarId&, Int inc);
  void incValue(VarId& v, Int val) { incValue(m_currentTime, v, val); }

  void setValue(const Timestamp&, VarId&, Int val);
  void setValue(VarId& v, Int val) { setValue(m_currentTime, v, val); }
  Int getValue(const Timestamp&, VarId&);
  Int getValue(VarId& v) { return getValue(m_currentTime, v); }

  Int getCommitedValue(VarId&);

  Timestamp getTmpTimestamp(VarId&);

  inline bool hasChanged(const Timestamp& t, VarId& v) const {
    return m_store.getConstIntVar(v).hasChanged(t);
  }
  void commit(VarId&);  // todo: this feels dangerous, maybe commit should
                        // always have a timestamp?
  void commitIf(const Timestamp&, VarId&);
  void commitValue(VarId&, Int val);

  //--------------------- Registration ---------------------
  /**
   * Register an invariant in the engine and return its pointer.
   * This also sets the id of the invariant to the new id.
   * @param args the constructor arguments of the invariant
   * @return the created invariant.
   */
  template <class T, typename... Args>
  std::enable_if_t<std::is_base_of<Invariant, T>::value, std::shared_ptr<T>>
  makeInvariant(Args&&... args) {
    if (!m_isOpen) {
      throw ModelNotOpenException(
          "Cannot make invariant when store is closed.");
    }
    auto invariantPtr = std::make_shared<T>(std::forward<Args>(args)...);

    auto newId = m_store.createInvariantFromPtr(invariantPtr);
    m_propGraph.registerInvariant(newId);
    invariantPtr->init(m_currentTime, *this);
    return invariantPtr;
  }

  //--------------------- Registration ---------------------
  /**
   * Register a constriant in the engine and return its pointer.
   * This also sets the id of the constraint to the new id.
   * @param args the constructor arguments of the constraint
   * @return the created constraint.
   */
  template <class T, typename... Args>
  std::enable_if_t<std::is_base_of<Constraint, T>::value, std::shared_ptr<T>>
  makeConstraint(Args&&... args) {
    if (!m_isOpen) {
      throw ModelNotOpenException(
          "Cannot make invariant when store is closed.");
    }
    auto constraintPtr = std::make_shared<T>(std::forward<Args>(args)...);

    auto newId = m_store.createInvariantFromPtr(constraintPtr);
    m_propGraph.registerInvariant(newId);
    constraintPtr->init(m_currentTime, *this);
    return constraintPtr;
  }

  /**
   * Creates an IntVar and registers it to the engine.
   * @return the created IntVar
   */
  VarId makeIntVar(Int initValue);

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

  inline const Store& getStore() { return m_store; }
  inline const Timestamp& getCurrentTime() { return m_currentTime; }

  inline BottomUpPropagationGraph& getPropGraph() { return m_propGraph; }
};