#pragma once

#include "core/propagationEngine.hpp"

namespace search {

class VariableStore {
 public:
  explicit VariableStore(const PropagationEngine& engine) : _engine(engine) {}

  [[nodiscard]] inline Int lowerBound(VarId variable) const {
    return _engine.lowerBound(variable);
  }

  [[nodiscard]] inline Int upperBound(VarId variable) const {
    return _engine.upperBound(variable);
  }

 private:
  const PropagationEngine& _engine;
};

}
