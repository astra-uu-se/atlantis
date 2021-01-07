#pragma once

#include <vector>

#include "core/savedInt.hpp"
#include "core/types.hpp"
class Engine;  // Forward declaration

class Invariant {
 private:
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

    NotificationQueue() : head(0), queue() {
      queue.push_back(0);
    }

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

  bool m_isPostponed;
  InvariantId m_id;
  // State used for returning next dependency. Null state is -1 by default
  SavedInt m_state;

  //  std::vector<bool> m_modifiedVars;
  NotificationQueue m_modifiedVars;

  VarId m_primaryOutput;
  std::vector<VarId> m_outputVars;

  explicit Invariant(Id t_id) : Invariant(t_id, -1) {}
  Invariant(Id t_id, Int nullState)
      : m_isPostponed(false),
        m_id(t_id),
        m_state(NULL_TIMESTAMP, nullState),
        m_modifiedVars(),
        m_outputVars() {}

  /**
   * Register to the engine that variable is defined by the invariant.
   * @param e the engine
   * @param v the id of the variable that is defined by the invariant.
   */
  void registerDefinedVariable(Engine& e, VarId v);

  /**
   * Used in Top-Down (Input-to-Output) propagation to notify that a 
   * variable local to the invariant has had its value changed. This 
   * method is called for each variable that was marked as modified 
   * in notify.
   * @param t the current timestamp
   * @param e the engine
   * @param id the local id of the variable.
   */
  virtual void notifyIntChanged(Timestamp t, Engine& e, LocalId id) = 0;

  /**
   * Updates the value of variable without queueing it for propagation
   */
  void updateValue(Timestamp t, Engine& e, VarId id, Int val);
  /**
   * Increases the value of variable without queueing it for propagation
   */
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

  /**
   * Used in Output-to-Input propagation to get the next input variable,
   * the next dependency, to visit.
   */
  virtual VarId getNextDependency(Timestamp, Engine& e) = 0;

  /**
   * Used in bottom-up (Output-to-Input) propagation to notify to the
   * invariant that the current dependency (the last dependency given by 
   * getNextDependency) has had its value changed.
   */
  virtual void notifyCurrentDependencyChanged(Timestamp, Engine& e) = 0;

  /**
   * Used in the Top-Down (Input-to-Output) propagation to notify that an
   * input variable has had its value changed.
   */
  void notify(LocalId id);

  /**
   * Used in the Top-Down (Input-to-Output) propagation when the invariant
   * has been notified of all modified input variables (dependencies) and
   * the primary and non-primary output variables (dependants) are to be
   * computed.
   */
  void compute(Timestamp t, Engine& e);

  virtual void commit(Timestamp, Engine&) { m_isPostponed = false; };
  inline void postpone() { m_isPostponed = true; }
  [[nodiscard]] inline bool isPostponed() const { return m_isPostponed; }

  inline VarId getPrimaryOutput() { return m_primaryOutput; }
  void queueNonPrimaryOutputVarsForPropagation(Timestamp t, Engine& e);
};
