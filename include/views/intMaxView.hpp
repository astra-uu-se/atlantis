#pragma once

#include <memory>
#include <vector>

#include "views/intView.hpp"

class IntMaxView : public IntView {
 private:
  Int _max;

 public:
  IntMaxView(VarId parentId, Int max) : IntView(parentId), _max(max) {}

  Int getValue(Timestamp) const override;
  Int getCommittedValue() const override;
  Int getLowerBound() const override;
  Int getUpperBound() const override;
};