#pragma once

#include <memory>
#include <vector>

#include "core/engine.hpp"
#include "views/intView.hpp"

class GreaterEqualConst : public IntView {
 private:
  const Int _val;

 public:
  explicit GreaterEqualConst(VarId parentId, Int val)
      : IntView(parentId), _val(val) {}

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};