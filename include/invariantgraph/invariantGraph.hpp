#pragma once

#include <unordered_map>
#include <vector>

#include "core/engine.hpp"
#include "search/neighbourhoods/neighbourhoodCombinator.hpp"
#include "structure.hpp"

namespace invariantgraph {

class InvariantGraphApplyResult {
 public:
  using VariableMap =
      std::unordered_map<VarId, fznparser::Identifier, VarIdHash>;

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

  [[nodiscard]] search::neighbourhoods::NeighbourhoodCombinator neighbourhood()
      const noexcept {
    std::vector<std::unique_ptr<search::neighbourhoods::Neighbourhood>>
        neighbourhoods;

    for (const auto& implicitContraint : _implicitConstraints) {
      neighbourhoods.push_back(implicitContraint->takeNeighbourhood());
    }

    return search::neighbourhoods::NeighbourhoodCombinator(
        std::move(neighbourhoods));
  }

  [[nodiscard]] VarId totalViolations() const noexcept {
    return _totalViolations;
  }

  [[nodiscard]] VarId objectiveVariable() const noexcept {
    return _objectiveVariable;
  }

  [[nodiscard]] std::unordered_map<fznparser::Identifier, VarId>
  invertedVariableMap() const {
    std::unordered_map<fznparser::Identifier, VarId> invertedMap(
        _variableMap.size());
    for (const auto& [varId, fznVar] : _variableMap) {
      invertedMap.emplace(fznVar, varId);
    }
    return invertedMap;
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

  InvariantGraph(const InvariantGraph&) = delete;
  InvariantGraph(InvariantGraph&&) = default;

  InvariantGraphApplyResult apply(Engine& engine);
};

}  // namespace invariantgraph