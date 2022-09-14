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