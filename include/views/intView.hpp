#pragma once


#include "core/types.hpp"
#include "views/view.hpp"

class Engine;  // Forward declaration

class IntView : public View {
 protected:
  friend class Engine;
  Engine* m_engine;  // TODO: a raw pointer might be the best option here as
                     // views lifetime depend on engine and not vice-versa.

 public:
  explicit IntView(VarId t_parentId) : View(t_parentId), m_engine(nullptr) {}

  void init(VarId id, Engine& e) {
    m_id = id;
    m_engine = &e;
  }

  virtual Int getValue(Timestamp t) = 0;
  virtual Int getCommittedValue() = 0;
  virtual Int getLowerBound() = 0;
  virtual Int getUpperBound() = 0;
};