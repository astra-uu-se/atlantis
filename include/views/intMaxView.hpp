#pragma once

#include <memory>
#include <vector>

#include "views/intView.hpp"

class IntMaxView : public IntView {
 private:
  Int _max;

 public:
  IntMaxView(const VarId parentId, Int max) : IntView(parentId), _max(max) {}

  Int getValue(Timestamp) override;
  Int getCommittedValue() override;
  Int getLowerBound() override;
  Int getUpperBound() override;
};