#pragma once

#include <assert.h>

#include <memory>
#include <queue>
#include <vector>

#include "core/intVar.hpp"
#include "core/invariant.hpp"
#include "core/tracer.hpp"
#include "core/types.hpp"
#include "exceptions/exceptions.hpp"
#include "propagation/propagationGraph.hpp"

class Engine {
 private:
  static const size_t ESTIMATED_NUM_OBJECTS = 1000;

  Timestamp m_currentTime;

  PropagationGraph m_propGraph;

  bool m_isOpen = true;

  class Store {
   private:
    std::vector<IntVar> m_intVars;
    std::vector<size_t> m_intVarIndexMap;
    std::vector<std::shared_ptr<Invariant>> m_invariants;

   public:
    Store(size_t estimatedSize, [[maybe_unused]] Id t_nullId) {
      m_intVars.reserve(estimatedSize);
      m_intVarIndexMap.reserve(estimatedSize);
      m_invariants.reserve(estimatedSize);

      m_intVars.emplace_back();
      m_intVarIndexMap.push_back(-1);
      m_invariants.push_back(nullptr);
    }
    [[nodiscard]] inline VarId createIntVar() {
      VarId newId = VarId(m_intVars.size());
      m_intVars.emplace_back(newId);
      m_intVarIndexMap.push_back(newId);
      return newId;
    }
    [[nodiscard]] inline InvariantId createInvariantFromPtr(std::shared_ptr<Invariant> ptr) {
      InvariantId newId = InvariantId(m_invariants.size());
      ptr->setId(newId);
      m_invariants.push_back(ptr);
      return newId;
    }
    inline IntVar& getIntVar(VarId& v) {
      return m_intVars.at(m_intVarIndexMap.at(v.id));
    }
    
    inline Invariant& getInvariant(InvariantId& i) {
      return *(m_invariants.at(i.id));
    }
    std::vector<IntVar>::iterator intVarBegin() {
      return m_intVars.begin()+1;
    }
    std::vector<IntVar>::iterator intVarEnd() {
      return m_intVars.end();
    }
    std::vector<std::shared_ptr<Invariant>>::iterator invariantBegin() {
      return m_invariants.begin()+1;
    }
    std::vector<std::shared_ptr<Invariant>>::iterator invariantEnd() {
      return m_invariants.end();
    }
  } m_store;


  void recomputeAndCommit();
 public:
  Engine(/* args */);

  //--------------------- Move semantics ---------------------
  void beginMove(Timestamp& t);
  void endMove(Timestamp& t);

  void open();
  void close();

  //--------------------- Notificaion ---------------------
  /***
   * @param t the timestamp when the changed happened
   * @param id the id of the changed variable
   */
  void notifyMaybeChanged(const Timestamp& t, VarId id);

  //--------------------- Variable ---------------------
  void incValue(const Timestamp&, VarId&, Int inc);
  void setValue(VarId& v, Int val);
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
   * @param args the constructor arguments of the invariant
   * @return the created invariant.
   */
  template <class T, typename... Args>
  std::enable_if_t<std::is_base_of<Invariant, T>::value, std::shared_ptr<T>>
  makeInvariant(Args&&... args) {
    if (!m_isOpen) {
      throw new ModelNotOpenException("Cannot make invariant when store is closed.");
    }
    auto invariantPtr = std::make_shared<T>(std::forward<Args>(args)...);

    auto newId = m_store.createInvariantFromPtr(invariantPtr);
    m_propGraph.registerInvariant(newId);
    invariantPtr->init(m_currentTime, *this);
    return invariantPtr;
  }

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