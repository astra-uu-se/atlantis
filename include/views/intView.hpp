#pragma once

#include "core/types.hpp"
#include "views/view.hpp"

class Engine;  // Forward declaration

class IntView : public View {
 protected:
  friend class Engine;
  Engine* _engine;  // TODO: a raw pointer might be the best option here as
                    // views lifetime depend on engine and not vice-versa.

 public:
  explicit IntView(VarId parentId) : View(parentId), _engine(nullptr) {}

  void init(VarId id, Engine& e) {
    _id = id;
    _engine = &e;
  }

  virtual Int getValue(Timestamp t) = 0;
  virtual Int getCommittedValue() = 0;
  virtual Int getLowerBound() = 0;
  virtual Int getUpperBound() = 0;
};