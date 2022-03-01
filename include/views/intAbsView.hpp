#pragma once

#include <memory>
#include <vector>

#include "views/intView.hpp"

class IntAbsView : public IntView {
 public:
  IntAbsView(const VarId parentId) : IntView(parentId) {}

  Int getValue(Timestamp) override;
  Int getCommittedValue() override;
  Int getLowerBound() override;
  Int getUpperBound() override;
};