#include "search/move.hpp"

void search::Move::apply(PropagationEngine& engine) {
  engine.beginMove();

  for (size_t idx = 0; idx < _targets.size(); idx++) {
    engine.setValue(_targets[idx], _values[idx]);
  }

  engine.endMove();
}