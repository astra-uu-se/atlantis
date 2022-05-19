#pragma once

#include "views/intView.hpp"

class ScalarView : public IntView {
 private:
  const Int _scalar;

 public:
  ScalarView(VarId parentId, Int scalar)
      : IntView(parentId), _scalar(scalar) {}

  [[nodiscard]] Int value(Timestamp) const override;
  [[nodiscard]] Int committedValue() const override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};