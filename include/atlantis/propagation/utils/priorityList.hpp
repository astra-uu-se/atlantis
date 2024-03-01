#pragma once

#include <vector>

#include "atlantis/propagation/variables/committable.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

class PriorityList {
 private:
  std::vector<Committable<Int>> _list;
  Committable<size_t> _minimum;
  Committable<size_t> _maximum;

 public:
  explicit PriorityList(size_t size);

  [[nodiscard]] size_t size() const noexcept;
  [[nodiscard]] Int minPriority(Timestamp ts) const noexcept;
  [[nodiscard]] Int maxPriority(Timestamp ts) const noexcept;

  void updatePriority(Timestamp ts, size_t idx, Int newValue);

  void commitIf(Timestamp ts);

 private:
  void computeMaximum(Timestamp ts);
  void computeMinimum(Timestamp ts);
};

}  // namespace atlantis::propagation
