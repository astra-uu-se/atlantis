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

  [[nodiscard]] Int getValue(Timestamp) const override;
  [[nodiscard]] Int getCommittedValue() const override;
  [[nodiscard]] Int getLowerBound() const override;
  [[nodiscard]] Int getUpperBound() const override;
};