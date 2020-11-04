#pragma once
#include <memory>

#include "core/intVar.hpp"
#include "core/savedInt.hpp"
#include "core/tracer.hpp"
#include "core/types.hpp"
#include "core/var.hpp"
#include "core/view.hpp"

class Engine;  // Forward declaration

class IntView : public View {
 protected:
  // SavedInt m_savedInt;

  friend class Engine;
  Engine* m_engine;  // TODO: a raw pointer might be the best option here as
                     // views lifetime depend on engine and not vice-versa.

 public:
  IntView(const VarId t_parentId) : View(t_parentId) {}
  virtual ~IntView() = default;

  void init(VarId id, Engine& e) {
    m_id = id;
    m_engine = &e;
  }

  virtual Int getValue(Timestamp t) = 0;
  virtual Int getCommittedValue() = 0;
  virtual Int getLowerBound() = 0;
  virtual Int getUpperBound() = 0;
};