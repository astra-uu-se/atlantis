#pragma once

#include <vector>

#include "core/savedInt.hpp"
#include "core/types.hpp"
class Engine;  // Forward declaration

class Invariant {
 private:
 protected:
  bool m_isPostponed;
  InvariantId m_id;
  // State used for returning next dependency. Null state is -1 by default
  SavedInt m_state;

  bool m_isModified;
  std::vector<bool> m_modifiedVars;

  VarId m_primaryOutput;
  std::vector<VarId> m_outputVars;

  explicit Invariant(Id t_id) : Invariant(t_id, -1) {}
  Invariant(Id t_id, Int nullState)
      : m_isPostponed(false),
        m_id(t_id),
        m_state(NULL_TIMESTAMP, nullState),
        m_isModified(false),
        m_modifiedVars(),
        m_outputVars() {}

  void registerDefinedVariable(Engine& e, VarId v);

  virtual void notifyIntChanged(Timestamp t, Engine& e, LocalId id) = 0;

  void updateValue(Timestamp t, Engine& e, VarId id, Int val);
  void incValue(Timestamp t, Engine& e, VarId id, Int val);

 public:
  virtual ~Invariant() = default;

  void setId(Id t_id) { m_id = t_id; }

  /**
   * Preconditions for initialisation:
   * 1) The invariant has been registered in an engine and has a valid ID.
   *
   * 2) All variables have valid ids (i.e., they have been
   * registered)
   *
   * Checklist for initialising an invariant:
   *
   *
   * 2) Register any output variables that are defined by this
   * invariant note that this can throw an exception if such a variable is
   * already defined.
   *
   * 3) Register dependency to any input variables.
   *
   * 4) Compute initial state of invariant!
   */
  virtual void init(Timestamp, Engine&) = 0;

  virtual void recompute(Timestamp, Engine&) = 0;

  virtual VarId getNextDependency(Timestamp, Engine& e) = 0;

  virtual void notifyCurrentDependencyChanged(Timestamp, Engine& e) = 0;

  void notify(Timestamp t, Engine& e, LocalId id);

  void compute(Timestamp t, Engine& e);

  virtual void commit(Timestamp, Engine&) { m_isPostponed = false; };
  inline void postpone() { m_isPostponed = true; }
  [[nodiscard]] inline bool isPostponed() const { return m_isPostponed; }
};