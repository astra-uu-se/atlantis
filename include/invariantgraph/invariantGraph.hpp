#pragma once

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "core/engine.hpp"
#include "structure.hpp"

namespace invariantgraph {

// Required because VarId implicitly converts to size_t, thereby ignoring the
// idType field. This means VarIds cannot be used as keys in a hashing
// container if views and intvars are mixed.
struct VarIdHash {
  std::size_t operator()(VarId const& varId) const noexcept {
    std::size_t typeHash = std::hash<int>{}(static_cast<int>(varId.idType));
    return typeHash ^ (varId.id << 1);
  }
};

class InvariantGraphApplyResult {
 public:
  using VariableMap =
      std::unordered_map<VarId, std::shared_ptr<fznparser::SearchVariable>,
                         VarIdHash>;

  InvariantGraphApplyResult(
      VariableMap variableMap,
      std::vector<ImplicitConstraintNode*> implicitConstraints,
      VarId totalViolations)
      : _variableMap(std::move(variableMap)),
        _implicitConstraints(std::move(implicitConstraints)),
        _totalViolations(totalViolations) {}

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

 private:
  VariableMap _variableMap;
  std::vector<ImplicitConstraintNode*> _implicitConstraints;
  VarId _totalViolations;
};

class InvariantGraph {
 private:
  std::vector<std::unique_ptr<VariableNode>> _variables;
  std::vector<std::unique_ptr<VariableDefiningNode>> _variableDefiningNodes;
  std::vector<ImplicitConstraintNode*> _implicitConstraints;

 public:
  InvariantGraph(
      std::vector<std::unique_ptr<VariableNode>> variables,
      std::vector<std::unique_ptr<VariableDefiningNode>> variableDefiningNodes)
      : _variables(std::move(variables)),
        _variableDefiningNodes(std::move(variableDefiningNodes)) {
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