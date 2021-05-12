#pragma once

#include <memory>
#include <vector>

#include "views/intView.hpp"

class IntOffsetView : public IntView {
 private:
  Int _offset;

 public:
  IntOffsetView(const VarId parentId, Int offset)
      : IntView(parentId), _offset(offset) {}

  Int getValue(Timestamp t) override;
  Int getCommittedValue() override;
  Int getLowerBound() override;
  Int getUpperBound() override;
};