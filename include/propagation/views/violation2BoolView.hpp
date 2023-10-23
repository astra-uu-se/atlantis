#pragma once

#include <memory>
#include <vector>

#include "propagation/engine.hpp"
#include "propagation/views/intView.hpp"

namespace atlantis::propagation {

class Violation2BoolView : public IntView {
 public:
  explicit Violation2BoolView(Engine& engine, const VarId parentId)
      : IntView(engine, parentId) {}

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override { return 0; }
  [[nodiscard]] Int upperBound() const override { return 1; }
};

}  // namespace atlantis::propagation