#pragma once

#include <memory>
#include <vector>

#include "views/intView.hpp"

class IntAbsView : public IntView {
 public:
  IntAbsView(const VarId parentId) : IntView(parentId) {}

  [[nodiscard]] Int value(Timestamp) const override;
  [[nodiscard]] Int committedValue() const override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};