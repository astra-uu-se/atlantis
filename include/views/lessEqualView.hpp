#pragma once

#include <memory>
#include <vector>

#include "core/engine.hpp"
#include "views/intView.hpp"

class LessEqualView : public IntView {
 private:
  const Int _val;

 public:
  explicit LessEqualView(VarId parentId, Int val)
      : IntView(parentId), _val(val) {}

  [[nodiscard]] Int value(Timestamp) const override;
  [[nodiscard]] Int committedValue() const override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};