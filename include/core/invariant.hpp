#pragma once

#include "core/propagationElement.hpp"

class Invariant : public PropagationElement {
 private:
  /* data */
 public:
  Invariant(Id t_id) : PropagationElement(t_id) {}
  ~Invariant() {}
};