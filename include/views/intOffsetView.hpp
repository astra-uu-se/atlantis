#pragma once

#include <memory>
#include <vector>

#include "views/intView.hpp"

class IntOffsetView : public IntView {
 private:
  const Int _offset;

 public:
  IntOffsetView(VarId parentId, Int offset)
      : IntView(parentId), _offset(offset) {}

  Int getValue(Timestamp) const override;
  Int getCommittedValue() const override;
  Int getLowerBound() const override;
  Int getUpperBound() const override;
};