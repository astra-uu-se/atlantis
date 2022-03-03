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
  const std::vector<std::unique_ptr<VariableNode>> _variables;
  const std::vector<std::unique_ptr<InvariantNode>> _invariants;
  const std::vector<std::unique_ptr<SoftConstraintNode>> _softConstraints;

  std::set<InvariantNode*> _appliedInvariants;
  std::set<SoftConstraintNode*> _appliedSoftConstraints;
  std::map<VariableNode*, VarId> _engineVariables;
  std::vector<VarId> _violationVars;

  friend class InvariantGraphBuilder;

 public:
  InvariantGraph(
      std::vector<std::unique_ptr<VariableNode>> variables,
      std::vector<std::unique_ptr<InvariantNode>> invariants,
      std::vector<std::unique_ptr<SoftConstraintNode>> softConstraints)
      : _variables(std::move(variables)),
        _invariants(std::move(invariants)),
        _softConstraints(std::move(softConstraints)) {}

  void apply(Engine& engine);

 private:
  void applyVariable(Engine& engine, VariableNode* node);
  void applyInvariant(Engine& engine, InvariantNode* node);
  void applyConstraint(Engine& engine, SoftConstraintNode* node);

  [[nodiscard]] Int totalViolationsUpperBound(Engine& engine) const;

  [[nodiscard]] bool wasVisited(VariableNode* node) const {
    return _engineVariables.count(node) > 0;
  }

  [[nodiscard]] bool wasVisited(InvariantNode* node) const {
    return _appliedInvariants.count(node) > 0;
  }

  [[nodiscard]] bool wasVisited(SoftConstraintNode* node) const {
    return _appliedSoftConstraints.count(node) > 0;
  }
};
}  // namespace invariantgraph