#pragma once

#include "core/propagationElement.hpp"

class Engine;  // Forward declaration

class Invariant : public PropagationElement {
 private:
  /* data */
 protected:
  Invariant(Id t_id) : PropagationElement(t_id) {}

 public:
  virtual ~Invariant() {}

  /**
   * Checklist for initialising an invariant:
   * 1) Register the invariant in the engine, this will set the invariants ID by
   * the engine.
   *
   * 2) Register any output variables that are defined by this
   * invariant note that this can throw an exception if such a variable is
   * already defined.
   *
   * 3) Register dependency to any input variables.
   *
   */
  virtual void init(Engine& e) = 0;
  virtual void notifyIntChanged(Timestamp& t, Engine& e, Id id, Int oldValue,
                                Int newValue, Int data) = 0;
  virtual void commit(Timestamp& t) = 0;
};