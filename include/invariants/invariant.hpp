#pragma once

#include <cassert>
#include <vector>

#include "core/types.hpp"
#include "variables/committable.hpp"
#include "variables/committableInt.hpp"

class Engine;  // Forward declaration

class Invariant {
 protected:
  Engine& _engine;
  /**
   * A simple queue structure of a fixed length to hold what input
   * variables that have been updated.
   */
  class NotificationQueue {
   public:
    void reserve(size_t size) {
      // This function should only be called during setup and need not be
      // efficient
      queue.resize(size);
      init();
    }

    void push(LocalId id) {
      assert(id < queue.size());
      if (queue[id] == id.id) {
        std::swap(queue[id], head);
      }
    }

    LocalId pop() {
      LocalId current(head);
      std::swap(head, queue[head]);
      return current;
    }

    [[nodiscard]] size_t size() const { return queue.size(); }

    [[nodiscard]] bool hasNext() const { return head < queue.size(); }

    NotificationQueue() : head(0), queue() { queue.push_back(0); }

   private:
    size_t head;
    std::vector<size_t> queue;

    void init() {
      for (size_t i = 0; i < queue.size(); ++i) {
        queue[i] = i;
      }
      head = queue.size();
    }
  };

  NotificationQueue _modifiedVars{};
  std::vector<VarId> _definedVars{};
  // State used for returning next input. Null state is -1 by default
  CommittableInt _state;
  VarId _primaryDefinedVar{NULL_ID};
  size_t _level{0};
  InvariantId _id{NULL_ID};
  bool _isPostponed{false};

  explicit Invariant(Engine& engine, Int nullState = -1)
      : _engine(engine), _state(NULL_TIMESTAMP, nullState) {}

  /**
   * Register to the engine that variable is defined by the invariant.
   * @param id the id of the variable that is defined by the invariant.
   */
  void registerDefinedVariable(VarId id);

  /**
   * Used in Input-to-Output propagation to notify that a
   * variable local to the invariant has had its value changed. This
   * method is called for each variable that was marked as modified
   * in notify.
   * @param ts the current timestamp
   * @param localId the local id of the variable.
   */
  virtual void notifyInputChanged(Timestamp ts, LocalId localId) = 0;

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
   * The total number of notifiable variables.
   */
  [[nodiscard]] size_t notifiableVarsSize() const {
    return _modifiedVars.size();
  }

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
   * 1) The invariant has been registered in an engine and has a valid ID.
   *
   * 2) All variables have valid ids (i.engine., they have been
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

  virtual void updateBounds(bool widenOnly = false) = 0;

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
   * Used in the Input-to-Output propagation to notify that an
   * input variable has had its value changed.
   */
  void notify(LocalId);

  /**
   * Used in the Input-to-Output propagation when the invariant
   * has been notified of all modified input variables and
   * the primary and non-primary defined variables are to be
   * computed.
   */
  void compute(Timestamp);

  virtual void commit(Timestamp) { _isPostponed = false; };

  inline void postpone() { _isPostponed = true; }
  [[nodiscard]] inline bool isPostponed() const { return _isPostponed; }

  [[nodiscard]] inline VarId primaryDefinedVar() const {
    return _primaryDefinedVar;
  }
  const inline std::vector<VarId>& nonPrimaryDefinedVars() const {
    return _definedVars;
  }
};
