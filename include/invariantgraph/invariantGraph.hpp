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

  std::set<InvariantNode*> appliedInvariants;
  std::set<SoftConstraintNode*> appliedSoftConstraints;
  std::map<VariableNode*, VarId> engineVariables;
  std::vector<VarId> violationVars;

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
    return engineVariables.count(node) > 0;
  }

  [[nodiscard]] bool wasVisited(InvariantNode* node) const {
    return appliedInvariants.count(node) > 0;
  }

  [[nodiscard]] bool wasVisited(SoftConstraintNode* node) const {
    return appliedSoftConstraints.count(node) > 0;
  }
};
}  // namespace invariantgraph