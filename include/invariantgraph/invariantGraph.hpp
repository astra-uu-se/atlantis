#pragma once

#include <deque>
#include <queue>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/engine.hpp"
#include "invariantgraph/implicitConstraintNode.hpp"
#include "invariantgraph/invariantGraphRoot.hpp"
#include "invariantgraph/softConstraintNode.hpp"
#include "invariantgraph/variableDefiningNode.hpp"
#include "invariantgraph/variableNode.hpp"
#include "invariants/linear.hpp"
#include "search/neighbourhoods/neighbourhoodCombinator.hpp"
#include "utils/fznAst.hpp"

namespace invariantgraph {

class InvariantGraphApplyResult {
 public:
  using VariableIdentifiers =
      std::unordered_map<VarId, std::string_view, VarIdHash>;

 private:
  VariableIdentifiers _variableIdentifiers;

  std::vector<ImplicitConstraintNode*> _implicitConstraints;
  VarId _totalViolations;
  VarId _objectiveVariable;

 public:
  InvariantGraphApplyResult(
      VariableIdentifiers variableIdentifiers,
      std::vector<ImplicitConstraintNode*> implicitConstraints,
      VarId totalViolations, VarId objectiveVariable)
      : _variableIdentifiers(std::move(variableIdentifiers)),
        _implicitConstraints(std::move(implicitConstraints)),
        _totalViolations(totalViolations),
        _objectiveVariable(objectiveVariable) {}

  [[nodiscard]] const VariableIdentifiers& variableIdentifiers()
      const noexcept {
    return _variableIdentifiers;
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
};

class InvariantGraph {
 private:
  std::vector<VariableNode> _variableNodes;

  std::unordered_map<std::string_view, VariableNode*> _namedVariableNodes;
  std::unordered_map<int64_t, VariableNode*> _intVariableNodes;
  std::array<VariableNode*, 2> _boolVariableNodes;

  std::vector<std::unique_ptr<VariableDefiningNode>> _variableDefiningNodes;
  std::vector<ImplicitConstraintNode*> _implicitConstraints;
  VariableNode* _objectiveVariable;

 public:
  InvariantGraph()
      : _variableNodes{VariableNode(SearchDomain({1}), false),
                       VariableNode(SearchDomain({0}), false)},
        _namedVariableNodes(),
        _intVariableNodes(),
        _boolVariableNodes{&_variableNodes.at(0), &_variableNodes.at(1)},
        _variableDefiningNodes(),
        _implicitConstraints(),
        _objectiveVariable(nullptr) {}

  InvariantGraph(const InvariantGraph&) = delete;
  InvariantGraph(InvariantGraph&&) = default;

  // TODO: This should be changed to be references and wrapped_reference
  // vectors!
  VariableNode* addVariable(bool);
  VariableNode* addVariable(const fznparser::BoolVar&);
  VariableNode* addVariable(std::reference_wrapper<const fznparser::BoolVar>);
  VariableNode* addVariable(const fznparser::BoolArg&);

  VariableNode* addVariable(Int);
  VariableNode* addVariable(const fznparser::IntVar&);
  VariableNode* addVariable(const fznparser::IntArg&);
  VariableNode* addVariable(std::reference_wrapper<const fznparser::IntVar>);

  std::vector<VariableNode*> addVariableArray(const fznparser::BoolVarArray&);
  std::vector<VariableNode*> addVariableArray(const fznparser::IntVarArray&);

  void splitMultiDefinedVariables();
  void breakCycles();

  InvariantGraphApplyResult apply(Engine& engine);

 private:
  std::vector<VariableNode*> findCycle(
      const std::unordered_map<VariableNode*, VariableNode*>& childOf,
      VariableNode* const node, VariableNode* const parent);

  std::pair<invariantgraph::VariableNode*,
            invariantgraph::VariableDefiningNode*>
  findPivotInCycle(const std::vector<VariableNode*>& cycle);

  std::vector<VariableNode*> breakCycles(
      VariableNode* node, std::unordered_set<VariableNode*>& visitedGlobal);
  VariableNode* breakCycle(const std::vector<VariableNode*>& cycle);

  void createVariables(Engine&);
  void createInvariants(Engine&);
  VarId createViolations(Engine&);
};

}  // namespace invariantgraph