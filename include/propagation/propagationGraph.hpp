#pragma once

#include <assert.h>

#include <memory>
#include <queue>
#include <vector>

#include "../core/types.hpp"

class PropagationGraph {
 protected:
  size_t m_numInvariants;
  size_t m_numVariables;

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
  /**
   * Map from InvariantId -> list of VarID
   *
   * Maps an invariant to all variables it depends on (its inputs).
   */
  std::vector<std::vector<VarId>> m_inputVariables;

  // Map from VarID -> vector of InvariantID
  std::vector<std::vector<InvariantId>> m_listeningInvariants;

  std::vector<bool> m_isOutputVar;
  std::vector<bool> m_isInputVar;

 public:
  PropagationGraph() : PropagationGraph(1000) {}
  PropagationGraph(size_t expectedSize);

  virtual ~PropagationGraph(){};

  virtual void notifyMaybeChanged(Timestamp t, VarId id) = 0;

  /**
   * update internal datastructures based on currently registered  variables and
   * invariants.
   */
  virtual void close();

  /**
   * Register an invariant in the propagation graph.
   */
  virtual void registerInvariant(InvariantId);

  /**
   * Register a variable in the propagation graph.
   */
  virtual void registerVar(VarId);

  /**
   * Register that Invariant to depends on variable from depends on dependency
   * @param depends the invariant that the variable depends on
   * @param source the depending variable
   */
  void registerInvariantDependsOnVar(InvariantId depends, VarId source);

  /**
   * Register that 'from' defines variable 'to'. Throws exception if
   * already defined.
   * @param depends the variable that is defined by the invariant
   * @param source the invariant defining the variable
   * @throw if the variable is already defined by an invariant.
   */
  void registerDefinedVariable(VarId depends, InvariantId source);

  // todo: Maybe there is a better word than "active", like "relevant".
  // --------------------- Activity ----------------
  /**
   * returns true if variable id is relevant for propagation.
   * Note that this is not the same thing as the variable being modified.
   */
  virtual bool isActive([[maybe_unused]] Timestamp t,
                        [[maybe_unused]] VarId id) {
    return true;
  }
  /**
   * returns true if invariant id is relevant for propagation.
   * Note that this is not the same thing as the invariant being modified.
   */
  virtual bool isActive([[maybe_unused]] Timestamp t,
                        [[maybe_unused]] InvariantId id) {
    return true;
  }

  /**
   * A stable variable is a variable that has been modified at timestamp t,
   * but will not be modified again (as all parent nodes are already up to
   * date).
   *
   * The variable returned by this method will be considered up to date for the
   * current timestamp.
   */
  [[nodiscard]] virtual VarId getNextStableVariable([
      [maybe_unused]] Timestamp t) = 0;

 inline size_t getNumVariables() {
    return m_numVariables;  // this ignores null var
  }

  inline size_t getNumInvariants() {
    return m_numInvariants;  // this ignores null invariant
  }

  inline bool isOutputVar(VarId id){
    return m_isOutputVar.at(id);
  }

  inline bool isInputVar(VarId id){
    return m_isInputVar.at(id);
  }

  inline const std::vector<VarId>& variableDefinedBy(InvariantId inv) const {
    return m_variablesDefinedByInvariant.at(inv);
  }
};
