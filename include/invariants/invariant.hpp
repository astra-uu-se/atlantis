#pragma once

#include <vector>

#include "core/types.hpp"
#include "variables/savedInt.hpp"
class Engine;  // Forward declaration

class Invariant {
 protected:
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
      if (queue[id] == id.id) {
        std::swap(queue[id], head);
      }
    }

    LocalId pop() {
      auto current = LocalId(head);
      std::swap(head, queue[head]);
      return current;
    }

    size_t size() { return queue.size(); }

    bool hasNext() { return head < queue.size(); }

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

  bool _isPostponed;
  InvariantId _id;
  // State used for returning next dependency. Null state is -1 by default
  SavedInt _state;

  //  std::vector<bool> _modifiedVars;
  NotificationQueue _modifiedVars;

  VarId _primaryOutput;
  std::vector<VarId> _outputVars;

  explicit Invariant(Id id) : Invariant(id, -1) {}
  Invariant(Id id, Int nullState)
      : _isPostponed(false),
        _id(id),
        _state(NULL_TIMESTAMP, nullState),
        _modifiedVars(),
        _outputVars() {}

  /**
   * Register to the engine that variable is defined by the invariant.
   * @param engine the engine
   * @param id the id of the variable that is defined by the invariant.
   */
  void registerDefinedVariable(Engine& engine, VarId id);

  /**
   * Used in Input-to-Output propagation to notify that a
   * variable local to the invariant has had its value changed. This
   * method is called for each variable that was marked as modified
   * in notify.
   * @param ts the current timestamp
   * @param engine the engine
   * @param localId the local id of the variable.
   */
  virtual void notifyIntChanged(Timestamp ts, Engine& engine,
                                LocalId localId) = 0;

  /**
   * Updates the value of variable without queueing it for propagation
   */
  void updateValue(Timestamp ts, Engine& engine, VarId id, Int val);
  /**
   * Increases the value of variable without queueing it for propagation
   */
  void incValue(Timestamp ts, Engine& engine, VarId id, Int val);

 public:
  virtual ~Invariant() = default;

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

  /**
   * Used in Output-to-Input propagation to get the next input variable,
   * the next dependency, to visit.
   */
  virtual VarId getNextDependency(Timestamp, Engine&) = 0;

  /**
   * Used in Output-to-Input propagation to notify to the
   * invariant that the current dependency (the last dependency given by
   * getNextDependency) has had its value changed.
   */
  virtual void notifyCurrentDependencyChanged(Timestamp, Engine&) = 0;

  /**
   * Used in the Input-to-Output propagation to notify that an
   * input variable has had its value changed.
   */
  void notify(LocalId);

  /**
   * Used in the Input-to-Output propagation when the invariant
   * has been notified of all modified input variables (dependencies) and
   * the primary and non-primary output variables (dependants) are to be
   * computed.
   */
  void compute(Timestamp, Engine&);

  virtual void commit(Timestamp, Engine&) { _isPostponed = false; };
  inline void postpone() { _isPostponed = true; }
  [[nodiscard]] inline bool isPostponed() const { return _isPostponed; }

  inline VarId getPrimaryOutput() { return _primaryOutput; }
  void queueNonPrimaryOutputVarsForPropagation(Timestamp ts, Engine& engine);
};
