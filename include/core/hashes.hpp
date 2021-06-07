#pragma once

#include <unordered_set>

#include "types.hpp"

namespace std {
template <>
struct hash<VarIdBase> {
  std::size_t operator()(const VarIdBase id) const noexcept { return id.id; }
};
}  // namespace std