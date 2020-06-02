#pragma once

#include "core/engine.hpp"
#include "core/propagationElement.hpp"

class Invariant : public PropagationElement {
 private:
  /* data */
 protected:
  Invariant(Id t_id) : PropagationElement(t_id) {}

public:
 virtual ~Invariant() {}
 virtual void notifyIntChanged(Engine& e, Id id, Int oldValue, Int newValue,
                               Int data) = 0;
};