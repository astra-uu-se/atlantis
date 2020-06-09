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

  PropagationGraph m_propGraph;

  class Store {
   private:
    std::vector<std::shared_ptr<IntVar>> m_intVars;
    std::vector<std::shared_ptr<Invariant>> m_invariants;

   public:
    Store(size_t estimatedSize, [[maybe_unused]] Id t_nullId) {
      m_intVars.reserve(estimatedSize);
      m_invariants.reserve(estimatedSize);

      m_intVars.push_back(nullptr);
      m_invariants.push_back(nullptr);
    }
    inline VarId createIntVar() {
      VarId newId = VarId(m_intVars.size());
      m_intVars.emplace_back(std::make_shared<IntVar>(newId));
      return newId;
    }
    inline InvariantId createInvariantFromPtr(std::shared_ptr<Invariant> ptr) {
      InvariantId newId = InvariantId(m_invariants.size());
      ptr->setId(newId);
      m_invariants.push_back(ptr);
      return newId;
    }
    inline IntVar& getIntVar(VarId& v) { return *(m_intVars.at(v.id)); }
    inline Invariant& getInvariant(InvariantId& i) {
      return *(m_invariants.at(i.id));
    }
  } m_store;

 public:
  Engine(/* args */);

  //--------------------- Move semantics ---------------------
  void beginMove(Timestamp& t);
  void endMove(Timestamp& t);

  //--------------------- Notificaion ---------------------
  /***
   * @param t the timestamp when the changed happened
   * @param id the id of the changed variable
   */
  void notifyMaybeChanged(const Timestamp& t, VarId id);

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
   * @param args the constructor arguments of the invariant
   * @return the created invariant.
   */
  template<class T, typename... Args>
  std::enable_if_t<std::is_base_of<Invariant, T>::value, std::shared_ptr<T>>
  makeInvariant(Args&& ...args) {
    auto invariantPtr = std::make_shared<T>(std::forward<Args>(args)...);
  
    InvariantId newId = InvariantId(m_invariants.size());
    invariantPtr->setId(newId);

    m_invariants.push_back(invariantPtr);
    m_variablesDefinedByInvariant.push_back({});
    assert(m_invariants.size() == m_variablesDefinedByInvariant.size());

  #ifdef VERBOSE_TRACE
  #include <iostream>
    std::cout << "Registering new invariant with id: " << newId << "\n";
  #endif

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