#pragma once

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "core/engine.hpp"
#include "structure.hpp"

namespace invariantgraph {
class InvariantGraph {
 private:
  std::vector<std::unique_ptr<VariableNode>> _variables;
  std::vector<std::unique_ptr<InvariantNode>> _invariants;
  std::vector<std::unique_ptr<SoftConstraintNode>> _softConstraints;

  std::map<VariableNode*, VarId> engineVariables{};
  std::vector<VarId> violationVars{};

  std::optional<VarId> _totalViolations{};
  std::optional<VarId> _objective{};

  friend class InvariantGraphBuilder;

 public:
  InvariantGraph(
      std::vector<std::unique_ptr<VariableNode>> variables,
      std::vector<std::unique_ptr<InvariantNode>> invariants,
      std::vector<std::unique_ptr<SoftConstraintNode>> softConstraints)
      : _variables(std::move(variables)),
        _invariants(std::move(invariants)),
        _softConstraints(std::move(softConstraints)) {}

  std::map<VarId, std::shared_ptr<fznparser::Variable>> apply(Engine& engine);

  [[nodiscard]] VarId totalViolations() const {
    assert(_totalViolations.has_value());

    return *_totalViolations;
  }

  [[nodiscard]] VarId objective() const {
    assert(_objective.has_value());

    return *_objective;
  }

 private:
  void applyVariable(Engine& engine, VariableNode* node);
  void applyInvariant(Engine& engine, InvariantNode* node);
  void applyConstraint(Engine& engine, SoftConstraintNode* node);

  [[nodiscard]] Int totalViolationsUpperBound(Engine& engine) const;
};
}  // namespace invariantgraph