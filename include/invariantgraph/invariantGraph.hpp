#pragma once

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "core/engine.hpp"
#include "structure.hpp"

namespace invariantgraph {

class InvariantGraphApplyResult {
 public:
  using VariableMap =
      std::unordered_map<VarId, std::shared_ptr<fznparser::SearchVariable>,
                         VarIdHash>;

  InvariantGraphApplyResult(
      VariableMap variableMap,
      std::vector<ImplicitConstraintNode*> implicitConstraints,
      VarId totalViolations, VarId objectiveVariable)
      : _variableMap(std::move(variableMap)),
        _implicitConstraints(std::move(implicitConstraints)),
        _totalViolations(totalViolations),
        _objectiveVariable(objectiveVariable) {}

  [[nodiscard]] const VariableMap& variableMap() const noexcept {
    return _variableMap;
  }

  [[nodiscard]] const std::vector<ImplicitConstraintNode*>&
  implicitConstraints() const noexcept {
    return _implicitConstraints;
  }

  [[nodiscard]] VarId totalViolations() const noexcept {
    return _totalViolations;
  }

  [[nodiscard]] VarId objectiveVariable() const noexcept {
    return _objectiveVariable;
  }

 private:
  VariableMap _variableMap;
  std::vector<ImplicitConstraintNode*> _implicitConstraints;
  VarId _totalViolations;
  VarId _objectiveVariable;
};

class InvariantGraph {
 private:
  std::vector<std::unique_ptr<VariableNode>> _variables;
  std::vector<std::unique_ptr<VariableDefiningNode>> _variableDefiningNodes;
  std::vector<ImplicitConstraintNode*> _implicitConstraints;
  VariableNode* _objectiveVariable;

 public:
  InvariantGraph(
      std::vector<std::unique_ptr<VariableNode>> variables,
      std::vector<std::unique_ptr<VariableDefiningNode>> variableDefiningNodes,
      VariableNode* objectiveVariable)
      : _variables(std::move(variables)),
        _variableDefiningNodes(std::move(variableDefiningNodes)),
        _objectiveVariable(objectiveVariable) {
    for (const auto& definingNode : _variableDefiningNodes) {
      if (auto implicitConstraint =
              dynamic_cast<ImplicitConstraintNode*>(definingNode.get())) {
        _implicitConstraints.push_back(implicitConstraint);
      }
    }
  }

  InvariantGraphApplyResult apply(Engine& engine);
};

}  // namespace invariantgraph