#pragma once

#include <memory>
#include <vector>

#include "views/intView.hpp"

class IntOffsetView : public IntView {
 private:
  const Int _offset;

 public:
  explicit IntOffsetView(VarId parentId, Int offset)
      : IntView(parentId), _offset(offset) {}

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};