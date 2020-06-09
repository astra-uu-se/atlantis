#pragma once

#include <assert.h>

#include <memory>
#include <queue>
#include <vector>

#include "core/intVar.hpp"
#include "core/invariant.hpp"
#include "core/tracer.hpp"
#include "core/types.hpp"

class Engine {
 private:
  static const size_t ESTIMATED_NUM_OBJECTS = 1000;

  Timestamp m_currentTime;

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

  /**
   * TODO: Delete these
   * Register an intVar in the engine and return its new id.
   * This also sets the id of the variable to the new id.
   */
  VarId registerIntVar(std::shared_ptr<IntVar>);
  void registerIntVar(std::vector<std::shared_ptr<IntVar>>);

 public:
  static const Id NULL_ID;
  Engine(/* args */);
  ~Engine();

  //--------------------- Move semantics ---------------------
  void beginMove(Timestamp& t);
  void endMove(Timestamp& t);

  //--------------------- Notificaion/Modification ---------------------
  /***
  * @param t the timestamp when the changed happened
  * @param id the id of the changed variable
  */
  void notifyMaybeChanged(const Timestamp& t, Id id);

  void incValue(const Timestamp&, IntVar&, Int inc);
  void setValue(const Timestamp&, IntVar&, Int val);

  void commit(IntVar&); //todo: this feels dangerous, maybe commit should always have a timestamp?
  void commitIf(const Timestamp&, IntVar&);
  void commitValue(const Timestamp&, IntVar&, Int val);

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
  std::shared_ptr<IntVar> makeIntVar();

  /**
   * Register that Invariant to depends on variable from depends on dependency
   * @param dependee the invariant that the variable depends on
   * @param source the depending variable
   * @param localId the id of the depending variable in the invariant
   * @param data additioonal data
   */
  void registerInvariantDependency(InvariantId dependee, VarId source,
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