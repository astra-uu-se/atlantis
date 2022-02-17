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
  std::vector<std::shared_ptr<VariableNode>> _variables;

  std::set<std::shared_ptr<InvariantNode>> appliedInvariants;
  std::set<std::shared_ptr<SoftConstraintNode>> appliedSoftConstraints;
  std::map<std::shared_ptr<VariableNode>, VarId> engineVariables;
  std::vector<VarId> violationVars;

  std::optional<VarId> _totalViolations;
  std::optional<VarId> _objective;

  friend class InvariantGraphBuilder;

 public:
  explicit InvariantGraph(std::vector<std::shared_ptr<VariableNode>> variables)
      : _variables(std::move(variables)) {}
  void apply(Engine& engine);

  [[nodiscard]] VarId totalViolations() const {
    assert(_totalViolations.has_value());

    return *_totalViolations;
  }

  [[nodiscard]] VarId objective() const {
    assert(_objective.has_value());

    return *_objective;
  }

 private:
  void applyVariable(Engine& engine, const std::shared_ptr<VariableNode>& node);
  void applyInvariant(Engine& engine,
                      const std::shared_ptr<InvariantNode>& node);
  void applyConstraint(Engine& engine,
                       const std::shared_ptr<SoftConstraintNode>& node);

  [[nodiscard]] Int totalViolationsUpperBound(Engine& engine) const;

  [[nodiscard]] bool wasVisited(
      const std::shared_ptr<VariableNode>& node) const {
    return engineVariables.count(node) > 0;
  }

  [[nodiscard]] bool wasVisited(
      const std::shared_ptr<InvariantNode>& node) const {
    return appliedInvariants.count(node) > 0;
  }

  [[nodiscard]] bool wasVisited(
      const std::shared_ptr<SoftConstraintNode>& node) const {
    return appliedSoftConstraints.count(node) > 0;
  }
};
}  // namespace invariantgraph