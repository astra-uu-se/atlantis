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
  fznparser::ProblemType _problemType;

  std::optional<VarId> _bound{};
  std::optional<VarId> _objective{};
  std::optional<VarId> _violation{};

 public:
  Objective(PropagationEngine& engine, fznparser::ProblemType problemType);

  VarId registerNode(VarId totalViolationId, VarId objectiveVarId);

  void tighten();

  [[nodiscard]] std::optional<VarId> bound() const noexcept { return _bound; }

 private:
  template <typename F>
  VarId registerOptimisation(VarId constraintViolation, VarId objectiveVarId,
                             Int initialBound, F constraintFactory) {
    auto lb = _engine.lowerBound(objectiveVarId);
    auto ub = _engine.upperBound(objectiveVarId);

    _bound = _engine.makeIntVar(initialBound, lb, ub);
    auto boundViolation =
        _engine.makeIntVar(0, 0, std::numeric_limits<Int>::max());
    constraintFactory(boundViolation, *_bound);

    if (constraintViolation == NULL_ID) {
      _violation = boundViolation;
    } else {
      _violation = _engine.makeIntVar(0, 0, std::numeric_limits<Int>::max());
      _engine.makeInvariant<Linear>(
          _engine, *_violation,
          std::vector<VarId>{boundViolation, constraintViolation});
    }
    return *_violation;
  }
};

}  // namespace search
