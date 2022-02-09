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
  std::set<std::shared_ptr<InvariantNode>> appliedInvariants;
  std::set<std::shared_ptr<SoftConstraintNode>> appliedSoftConstraints;
  std::map<std::shared_ptr<VariableNode>, VarId> engineVariables;
  std::set<std::shared_ptr<VariableNode>> violationVars;

  friend class InvariantGraphBuilder;

 public:
  void apply(Engine& engine);

 private:
  void applyVariable(Engine& engine, const std::shared_ptr<VariableNode>& node);
  void applyInvariant(Engine& engine,
                      const std::shared_ptr<InvariantNode>& node);
  void applyConstraint(Engine& engine,
                       const std::shared_ptr<SoftConstraintNode>& node);

  [[nodiscard]] Int totalViolationsUpperBound(Engine& engine) const;
  [[nodiscard]] std::vector<VarId> getViolationVariables() const;

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