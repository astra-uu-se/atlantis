#pragma once

#include "core/propagationNode.hpp"

class Engine;  // Forward declaration

class Invariant : public PropagationNode {
 private:
  /* data */
 protected:
  Invariant(Id t_id) : PropagationNode(t_id) {}

 public:
  virtual ~Invariant() {}

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
  virtual void init(const Timestamp&, Engine&) = 0;

  virtual void recompute(const Timestamp&, Engine&) = 0;

  /**
   * Precondition: oldValue != newValue
   */
  virtual void notifyIntChanged(const Timestamp& t, Engine& e, LocalId id,
                                Int oldValue, Int newValue, Int data) = 0;

  // TODO: This commit is somehow different from other commits as it just
  // forwards the commit call and validates the node. Maybe remove and let
  // engine do this by looking at defined variables of invariant...
  virtual void commit(const Timestamp& t, Engine&) = 0;
};