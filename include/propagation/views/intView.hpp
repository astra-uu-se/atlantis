#pragma once

#include "types.hpp"
#include "propagation/views/view.hpp"

namespace atlantis::propagation {

class Engine;  // Forward declaration

class IntView : public View {
 protected:
  friend class Engine;
  // A raw pointer might be the best option here as views lifetime depend
  // on engine and not vice-versa:

 public:
  explicit IntView(Engine& engine, VarId parentId) : View(engine, parentId) {}

  void init(VarId id) { _id = id; }

  [[nodiscard]] virtual Int value(Timestamp) = 0;
  [[nodiscard]] virtual Int committedValue() = 0;
  [[nodiscard]] virtual Int lowerBound() const = 0;
  [[nodiscard]] virtual Int upperBound() const = 0;
};

}  // namespace atlantis::propagation