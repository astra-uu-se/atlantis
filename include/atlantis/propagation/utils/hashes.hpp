#pragma once

#include <unordered_set>

#include "atlantis/types.hpp"

namespace std {
template <>
struct hash<atlantis::propagation::VarIdBase> {
  std::size_t operator()(
      const atlantis::propagation::VarIdBase id) const noexcept {
    return id.id;
  }
};
}  // namespace std
