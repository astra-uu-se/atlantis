#pragma once

#include "core/types.hpp"
#include "views/view.hpp"

class Engine;  // Forward declaration

class IntView : public View {
 protected:
  friend class Engine;
  const Engine*
      _engine;  // TODO: a raw pointer might be the best option here as
                // views lifetime depend on engine and not vice-versa.

 public:
  explicit IntView(VarId parentId) : View(parentId), _engine(nullptr) {}

  void init(VarId id, const Engine& engine) {
    _id = id;
    _engine = &engine;
  }

  [[nodiscard]] virtual Int getValue(Timestamp) const = 0;
  [[nodiscard]] virtual Int getCommittedValue() const = 0;
  [[nodiscard]] virtual Int getLowerBound() const = 0;
  [[nodiscard]] virtual Int getUpperBound() const = 0;
};