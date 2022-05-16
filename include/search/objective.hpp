#pragma once

#include <limits>
#include <utility>

#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "fznparser/model.hpp"
#include "invariants/linear.hpp"

namespace search {

class Objective {
 private:
  PropagationEngine& _engine;
  fznparser::Objective _modelObjective;

  std::optional<VarId> _bound{};
  std::optional<VarId> _objective{};
  std::optional<VarId> _violation{};

 public:
  Objective(PropagationEngine& engine, const fznparser::FZNModel& model);

  VarId registerWithEngine(VarId constraintViolation, VarId objectiveVariable);

  void tighten();

  [[nodiscard]] std::optional<VarId> bound() const noexcept { return _bound; }

 private:
  template <typename F>
  VarId registerOptimisation(VarId constraintViolation, VarId objectiveVariable,
                             Int initialBound, F constraintFactory) {
    auto lb = _engine.lowerBound(objectiveVariable);
    auto ub = _engine.upperBound(objectiveVariable);

    _bound = _engine.makeIntVar(initialBound, lb, ub);
    auto boundViolation =
        _engine.makeIntVar(0, 0, std::numeric_limits<Int>::max());
    constraintFactory(boundViolation, *_bound);

    if (constraintViolation == NULL_ID) {
      _violation = boundViolation;
    } else {
      _violation = _engine.makeIntVar(0, 0, std::numeric_limits<Int>::max());
      _engine.makeInvariant<Linear>(
          std::vector<VarId>{boundViolation, constraintViolation}, *_violation);
    }
    return *_violation;
  }
};

}  // namespace search
