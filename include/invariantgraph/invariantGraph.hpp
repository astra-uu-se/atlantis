#pragma once

#include <deque>
#include <queue>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/engine.hpp"
#include "invariantgraph/constraints/allEqualNode.hpp"
#include "invariantgraph/constraints/boolEqNode.hpp"
#include "invariantgraph/constraints/intEqNode.hpp"
#include "invariantgraph/implicitConstraintNode.hpp"
#include "invariantgraph/invariantGraphRoot.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "invariantgraph/softConstraintNode.hpp"
#include "invariantgraph/variableNode.hpp"
#include "invariants/linear.hpp"
#include "search/neighbourhoods/neighbourhoodCombinator.hpp"
#include "utils/fznAst.hpp"

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

 private:
  VariableMap _variableMap;
  std::vector<ImplicitConstraintNode*> _implicitConstraints;
  VarId _totalViolations;
  VarId _objectiveVariable;
};

class InvariantGraph {
 private:
  std::vector<std::unique_ptr<VariableNode>> _variables;
  std::vector<VariableNode*> _valueNodes;
  std::vector<std::unique_ptr<InvariantNode>> _invariantNodes;
  std::vector<ImplicitConstraintNode*> _implicitConstraints;
  VariableNode* _objectiveVariable;

 public:
  InvariantGraph(std::vector<std::unique_ptr<VariableNode>> variables,
                 std::vector<VariableNode*> valueNodes,
                 std::vector<std::unique_ptr<InvariantNode>> invariantNodes,
                 VariableNode* objectiveVariable)
      : _variables(std::move(variables)),
        _valueNodes(std::move(valueNodes)),
        _invariantNodes(std::move(invariantNodes)),
        _objectiveVariable(objectiveVariable) {
    for (const auto& definingNode : _invariantNodes) {
      if (auto implicitConstraint =
              dynamic_cast<ImplicitConstraintNode*>(definingNode.get())) {
        _implicitConstraints.push_back(implicitConstraint);
      }
    }
#ifndef NDEBUG
    if (_objectiveVariable != nullptr) {
      bool containsObj = false;
      for (auto& var : _variables) {
        if (var.get() == _objectiveVariable) {
          containsObj = true;
          break;
        }
      }
      assert(containsObj);
    }
#endif
  }

  InvariantGraph(const InvariantGraph&) = delete;
  InvariantGraph(InvariantGraph&&) = default;

  void splitMultiDefinedVariables();
  void breakCycles();

  InvariantGraphApplyResult apply(Engine& engine);

 private:
  std::pair<invariantgraph::VariableNode*, invariantgraph::InvariantNode*>
  findPivotInCycle(
      const std::vector<std::pair<VariableNode*, InvariantNode*>>& cycle);

  std::vector<VariableNode*> breakCycles(
      VariableNode* variable,
      std::unordered_map<VariableNode*, std::unordered_set<InvariantNode*>>&
          visitedGlobal);
  VariableNode* breakCycle(
      const std::vector<std::pair<VariableNode*, InvariantNode*>>& cycle);

  void createVariables(Engine&);
  void createInvariants(Engine&);
  VarId createViolations(Engine&);
};

}  // namespace invariantgraph