#pragma once

#include <cstddef>

namespace atlantis::invariantgraph::rank {
inline constexpr size_t IMPLICIT_RANK_BOOL_LIN_LE = 1000;
inline constexpr size_t IMPLICIT_RANK_BOOL_LIN_EQ = 2000;
inline constexpr size_t IMPLICIT_RANK_INT_LIN_LE = 1500;
inline constexpr size_t IMPLICIT_RANK_INT_LIN_EQ = 2500;
inline constexpr size_t IMPLICIT_RANK_COUNT = 3000;
inline constexpr size_t IMPLICIT_RANK_ALL_DIFFERENT = 4000;
inline constexpr size_t IMPLICIT_RANK_TABLE = 5000;
inline constexpr size_t IMPLICIT_RANK_CIRCUIT = 6000;


}  // namespace atlantis::invariantgraph
