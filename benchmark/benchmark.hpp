#pragma once

#include "core/propagationEngine.hpp"
#include "core/types.hpp"

inline PropagationMode intToPropagationMode(int state) {
  switch (state) {
    case 3:
    case 2:
    case 1:
      return PropagationMode::OUTPUT_TO_INPUT;
    case 0:
    default:
      return PropagationMode::INPUT_TO_OUTPUT;
  }
}

inline OutputToInputMarkingMode intToOutputToInputMarkingMode(int state) {
  switch (state) {
    case 3:
      return OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION;
    case 2:
      return OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC;
    case 1:
    case 0:
    default:
      return OutputToInputMarkingMode::NONE;
  }
}

inline void setEngineModes(PropagationEngine& engine, const int state) {
  engine.setPropagationMode(intToPropagationMode(state));
  engine.setOutputToInputMarkingMode(intToOutputToInputMarkingMode(state));
}

inline size_t rand_in_range(size_t minInclusive, size_t maxInclusive,
                            std::mt19937& rng) {
  return std::uniform_int_distribution<size_t>(minInclusive, maxInclusive)(rng);
}

inline bool all_in_range(size_t minInclusive, size_t maxExclusive,
                         std::function<bool(size_t)> predicate) {
  std::vector<size_t> vec(maxExclusive - minInclusive);
  std::iota(vec.begin(), vec.end(), minInclusive);
  return std::all_of(vec.begin(), vec.end(), predicate);
}

inline void defaultArguments(benchmark::internal::Benchmark* benchmark) {
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