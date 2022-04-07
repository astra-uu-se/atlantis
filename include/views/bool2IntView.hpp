#pragma once

#include <memory>
#include <vector>

#include "views/intView.hpp"

/**
 * In MiniZinc, the bool2int constraint enforces the following mapping:
 *  - 1 <-> true
 *  - 0 <-> false
 *
 *  However, to make it easier to use booleans in the violation of the search,
 *  we would like the mapping
 *  - 0 <-> true
 *  - 1 <-> false
 *
 *  This view can be used to ensure the MiniZinc mapping is used when the
 *  defined integer is the input for other constraints, where we want to use the
 *  MiniZinc mapping.
 */
class Bool2IntView : public IntView {
 public:
  explicit Bool2IntView(const VarId parentId) : IntView(parentId) {}

  [[nodiscard]] Int value(Timestamp) const override;
  [[nodiscard]] Int committedValue() const override;
  [[nodiscard]] Int lowerBound() const override { return 0; }
  [[nodiscard]] Int upperBound() const override { return 1; }
};
