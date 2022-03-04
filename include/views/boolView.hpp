#pragma once

#include <memory>
#include <vector>

#include "views/intView.hpp"

class BoolView : public IntView {
 public:
  explicit BoolView(const VarId parentId) : IntView(parentId) {}

  Int getValue(Timestamp) override;
  Int getCommittedValue() override;
  Int getLowerBound() override { return 0; }
  Int getUpperBound() override { return 1; }
};