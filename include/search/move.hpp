#pragma once

#include "assignment.hpp"
#include "cost.hpp"

namespace search {

class Move {
 public:
  virtual ~Move() = default;

  /**
   * Probe the cost of this move on the given assignment. Will only probe the
   * assignment once.
   *
   * @param assignment The assignment to probe on.
   * @return The cost of the assignment if this move were committed.
   */
  const Cost& probe(const Assignment& assignment);

  /**
   * Commit this move on the given assignment.
   *
   * @param assignment The assignment to change.
   */
  void commit(Assignment& assignment);

 protected:
  virtual void modify(AssignmentModification& modifications) = 0;

 private:
  Cost _cost{0, 0};
  bool _probed{false};
};

class AssignMove : public Move {
 public:
  AssignMove(VarId target, Int value) : _target(target), _value(value) {}

 protected:
  void modify(AssignmentModification& modifications) override;

 private:
  VarId _target;
  Int _value;
};

}  // namespace search
