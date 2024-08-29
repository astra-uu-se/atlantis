#pragma once

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/types.hpp"

namespace atlantis::benchmark {

inline int int_pow(int base, int exponent) {
  int power = 1;
  for (int i = 0; i < exponent; ++i) {
    power = power * base;
  }
  return power;
}

inline propagation::PropagationMode intToPropagationMode(int state) {
  switch (state) {
    case 3:
    case 2:
    case 1:
      return propagation::PropagationMode::OUTPUT_TO_INPUT;
    case 0:
    default:
      return propagation::PropagationMode::INPUT_TO_OUTPUT;
  }
}

inline propagation::OutputToInputMarkingMode intToOutputToInputMarkingMode(
    int state) {
  switch (state) {
    case 3:
      return propagation::OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION;
    case 2:
      return propagation::OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC;
    case 1:
    case 0:
    default:
      return propagation::OutputToInputMarkingMode::NONE;
  }
}

inline void setSolverMode(propagation::Solver& solver, const int state) {
  solver.setPropagationMode(intToPropagationMode(state));
  solver.setOutputToInputMarkingMode(intToOutputToInputMarkingMode(state));
}

inline size_t rand_in_range(size_t minInclusive, size_t maxInclusive,
                            std::mt19937& rng) {
  return std::uniform_int_distribution<size_t>(minInclusive, maxInclusive)(rng);
}

inline bool all_in_range(size_t minInclusive, size_t maxExclusive,
                         std::function<bool(size_t)>&& predicate) {
  std::vector<size_t> vec(maxExclusive - minInclusive);
  std::iota(vec.begin(), vec.end(), minInclusive);
  return std::all_of(vec.begin(), vec.end(), std::move(predicate));
}

inline void defaultArguments(::benchmark::internal::Benchmark* benchmark) {
  int n = 16;
  while (n <= 1024) {
    for (int mode = 0; mode <= 3; ++mode) {
      benchmark->Args({n, mode});
    }
    if (n < 32) {
      n += 16;
    } else if (n < 128) {
      n += 32;
    } else if (n < 256) {
      n += 64;
    } else {
      n *= 2;
    }
#ifndef NDEBUG
    return;
#endif
  }
}

inline void treeArguments(::benchmark::internal::Benchmark* benchmark) {
  int outDegree = 2;
  while (outDegree <= 10) {
    int treeHeight = (outDegree >= 4 ? 1 : 2);
    while (treeHeight <= 10) {
      const int pow = int_pow(outDegree, treeHeight);
      if (pow > 1024) {
        break;
      }
      for (int mode = 0; mode <= 3; ++mode) {
        benchmark->Args({outDegree, treeHeight, mode});
      }
#ifndef NDEBUG
      return;
#endif
      treeHeight += (outDegree <= 4 ? 2 : 1);
    }
    outDegree += 2;
  }
}

}  // namespace atlantis::benchmark
