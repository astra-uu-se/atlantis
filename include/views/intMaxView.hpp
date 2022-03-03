#pragma once

#include <memory>
#include <vector>

#include "views/intView.hpp"

class IntMaxView : public IntView {
 private:
  Int _max;

 public:
  IntMaxView(VarId parentId, Int max) : IntView(parentId), _max(max) {}

  [[nodiscard]] Int getValue(Timestamp) const override;
  [[nodiscard]] Int getCommittedValue() const override;
  [[nodiscard]] Int getLowerBound() const override;
  [[nodiscard]] Int getUpperBound() const override;
};