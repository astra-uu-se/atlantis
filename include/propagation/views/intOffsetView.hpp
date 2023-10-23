#pragma once

#include <memory>
#include <vector>

#include "propagation/engine.hpp"
#include "propagation/views/intView.hpp"

namespace atlantis::propagation {

class IntOffsetView : public IntView {
 private:
  const Int _offset;

 public:
  explicit IntOffsetView(Engine& engine, VarId parentId, Int offset)
      : IntView(engine, parentId), _offset(offset) {}

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation