#pragma once

#include <memory>
#include <vector>

#include "views/intView.hpp"

class BoolView : public IntView {
 public:
  explicit BoolView(const VarId parentId) : IntView(parentId) {}

  [[nodiscard]] Int value(Timestamp) const override;
  [[nodiscard]] Int committedValue() const override;
  [[nodiscard]] Int lowerBound() const override { return 0; }
  [[nodiscard]] Int upperBound() const override { return 1; }
};