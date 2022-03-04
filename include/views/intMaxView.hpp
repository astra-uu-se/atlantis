#pragma once

#include <memory>
#include <vector>

#include "views/intView.hpp"

class IntMaxView : public IntView {
 private:
  Int _max;

 public:
  IntMaxView(VarId parentId, Int max) : IntView(parentId), _max(max) {}

  [[nodiscard]] Int value(Timestamp) const override;
  [[nodiscard]] Int committedValue() const override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};