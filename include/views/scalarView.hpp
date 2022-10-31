#pragma once

#include "views/intView.hpp"

class ScalarView : public IntView {
 private:
  const Int _scalar;

 public:
  explicit ScalarView(Engine& engine, VarId parentId, Int scalar)
      : IntView(engine, parentId), _scalar(scalar) {}

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};