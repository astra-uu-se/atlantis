#pragma once

#include <cassert>
#include <vector>

#include "atlantis/propagation/types.hpp"
#include "atlantis/propagation/variables/committableInt.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

class SolverBase;  // Forward declaration

class Invariant {
 protected:
  SolverBase& _solver;

  std::vector<VarId> _definedVars{};
  // State used for returning next input. Null state is -1 by default
  CommittableInt _state;
  VarId _primaryDefinedVar{NULL_ID};
  size_t _level{0};
  InvariantId _id{NULL_ID};
  bool _isPostponed{false};

  explicit Invariant(SolverBase& solver, Int nullState = -1)
      : _solver(solver), _state(NULL_TIMESTAMP, nullState) {}

  /**
   * Register to the solver that variable is defined by the invariant.
   * @param id the id of the variable that is defined by the invariant.
   */
  void registerDefinedVar(VarId id);

  /**
   * Updates the value of variable without queueing it for propagation
   */
  void updateValue(Timestamp ts, VarId id, Int val);
  /**
   * Increases the value of variable without queueing it for propagation
   */
  void incValue(Timestamp ts, VarId id, Int val);

 public:
  virtual ~Invariant() = default;

  /**
   * @brief The level of the invariant in the invariant graph
   */
  [[nodiscard]] size_t level() const noexcept { return _level; }
  void setLevel(size_t newLevel) noexcept { _level = newLevel; }

  [[nodiscard]] virtual VarId dynamicInputVar(Timestamp) const noexcept {
    return NULL_ID;
  }

  [[nodiscard]] inline InvariantId id() const noexcept { return _id; }

  void setId(Id id) { _id = id; }

  /**
   * Preconditions for initialisation:
   * 1) The invariant has been registered in an solver and has a valid ID.
   *
   * 2) All variables have valid ids (i.solver., they have been
   * registered)
   *
   * Checklist for initialising an invariant:
   *
   *
   * 2) Register all defined variables that are defined by this
   * invariant. note that this can throw an exception if such a variable is
   * already defined by another invariant.
   *
   * 3) Register all variable inputs.
   *
   * 4) Compute initial state of invariant!
   */
  virtual void registerVars() = 0;

  virtual void updateBounds(bool widenOnly) = 0;

  virtual void close(Timestamp){};

  virtual void recompute(Timestamp) = 0;

  /**
   * Used in Output-to-Input propagation to get the next input variable to
   * visit.
   */
  virtual VarId nextInput(Timestamp) = 0;

  /**
   * Used in Output-to-Input propagation to notify to the
   * invariant that the current input (the last input given by
   * nextInput) has had its value changed.
   */
  virtual void notifyCurrentInputChanged(Timestamp) = 0;

  /**
   * Used in Input-to-Output propagation to notify that a
   * variable local to the invariant has had its value changed. This
   * method is called for each variable that was marked as modified
   * in notify.
   * @param ts the current timestamp
   * @param localId the local id of the variable.
   */
  virtual void notifyInputChanged(Timestamp ts, LocalId localId) = 0;

  virtual void commit(Timestamp) { _isPostponed = false; };

  inline void postpone() { _isPostponed = true; }
  [[nodiscard]] inline bool isPostponed() const { return _isPostponed; }

  [[nodiscard]] inline VarId primaryDefinedVar() const {
    return _primaryDefinedVar;
  }
  [[nodiscard]] const inline std::vector<VarId>& nonPrimaryDefinedVars() const {
    return _definedVars;
  }
};

}  // namespace atlantis::propagation
