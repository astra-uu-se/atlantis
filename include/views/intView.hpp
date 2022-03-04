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

  [[nodiscard]] virtual Int value(Timestamp) const = 0;
  [[nodiscard]] virtual Int committedValue() const = 0;
  [[nodiscard]] virtual Int lowerBound() const = 0;
  [[nodiscard]] virtual Int upperBound() const = 0;
};