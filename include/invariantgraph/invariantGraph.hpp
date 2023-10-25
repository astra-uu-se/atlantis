#pragma once

#include <deque>
#include <queue>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "invariantgraph/implicitConstraintNode.hpp"
#include "invariantgraph/invariantGraphRoot.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "invariantgraph/types.hpp"
#include "invariantgraph/varNode.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/invariants/linear.hpp"
#include "propagation/solver.hpp"
#include "search/neighbourhoods/neighbourhoodCombinator.hpp"
#include "utils/fznAst.hpp"

namespace atlantis::invariantgraph {

class InvariantGraphApplyResult {
 public:
  using VarIdentifiers = std::unordered_map<propagation::VarId, std::string,
                                            propagation::VarIdHash>;

 private:
  VarIdentifiers _varIdentifiers;

  std::vector<std::unique_ptr<ImplicitConstraintNode>> _implicitConstraintNodes;
  propagation::VarId _totalViolationId;
  propagation::VarId _objectiveVarId;

 public:
  InvariantGraphApplyResult(
      VarIdentifiers&& varIdentifiers,
      std::vector<std::unique_ptr<ImplicitConstraintNode>>&&
          implicitConstraints,
      propagation::VarId totalViolationId, propagation::VarId objectiveVarId)
      : _varIdentifiers(std::move(varIdentifiers)),
        _implicitConstraintNodes(std::move(implicitConstraints)),
        _totalViolationId(totalViolationId),
        _objectiveVarId(objectiveVarId) {}

  [[nodiscard]] const VarIdentifiers& varIdentifiers() const noexcept {
    return _varIdentifiers;
  }

  [[nodiscard]] search::neighbourhoods::NeighbourhoodCombinator neighbourhood()
      const noexcept {
    std::vector<std::shared_ptr<search::neighbourhoods::Neighbourhood>>
        neighbourhoods;

    for (auto const& implicitContraint : _implicitConstraintNodes) {
      std::shared_ptr<search::neighbourhoods::Neighbourhood> neighbourhood =
          implicitContraint->neighbourhood();
      if (neighbourhood != nullptr) {
        neighbourhoods.push_back(std::move(neighbourhood));
      }
    }

    return search::neighbourhoods::NeighbourhoodCombinator(
        std::move(neighbourhoods));
  }

  [[nodiscard]] propagation::VarId totalViolationId() const noexcept {
    return _totalViolationId;
  }

  [[nodiscard]] propagation::VarId objectiveVarId() const noexcept {
    return _objectiveVarId;
  }
};

class InvariantGraph {
 private:
  std::vector<VarNode> _varNodes;
  std::unordered_map<std::string, VarNodeId> _namedVarNodeIndices;
  std::unordered_map<Int, VarNodeId> _intVarNodeIndices;
  std::array<VarNodeId, 2> _boolVarNodeIndices;

  std::vector<std::unique_ptr<InvariantNode>> _invariantNodes;
  std::vector<std::unique_ptr<ImplicitConstraintNode>> _implicitConstraintNodes;
  VarNodeId _objectiveVarNodeId;

  std::unordered_map<std::string, VarNodeId> _outputVars;
  std::unordered_map<std::string, std::vector<VarNodeId>> _outputArrays;

  void populateRootNode();

 public:
  InvariantGraph();

  InvariantGraph(const InvariantGraph&) = delete;
  InvariantGraph(InvariantGraph&&) = default;

  // TODO: This should be changed to be references and wrapped_reference
  // vectors!
  VarNodeId createVarNode(bool);
  VarNodeId createVarNode(const fznparser::BoolVar&);
  VarNodeId createVarNode(std::reference_wrapper<const fznparser::BoolVar>);
  VarNodeId createVarNode(const fznparser::BoolArg&);

  VarNodeId createVarNode(Int);
  VarNodeId createVarNode(const fznparser::IntVar&);
  VarNodeId createVarNode(const fznparser::IntArg&);
  VarNodeId createVarNode(std::reference_wrapper<const fznparser::IntVar>);

  VarNodeId createVarNode(const SearchDomain&, bool isIntVar);
  VarNodeId createVarNode(const VarNode&);

  std::vector<VarNodeId> createVarNodes(const fznparser::BoolVarArray&);
  std::vector<VarNodeId> createVarNodes(const fznparser::IntVarArray&);

  VarNode& varNode(const std::string& identifier);
  VarNode& varNode(VarNodeId id);

  propagation::VarId varId(const std::string& identifier) const;
  propagation::VarId varId(VarNodeId id) const;

  InvariantNode& invariantNode(InvariantNodeId);
  InvariantGraphRoot& root();
  ImplicitConstraintNode& implicitConstraintNode(InvariantNodeId);

  InvariantNodeId nextInvariantNodeId() const noexcept;
  InvariantNodeId nextImplicitNodeId() const noexcept;

  InvariantNodeId addInvariantNode(std::unique_ptr<InvariantNode>&&);

  InvariantNodeId addImplicitConstraintNode(
      std::unique_ptr<ImplicitConstraintNode>&&);

  void splitMultiDefinedVars();
  void breakCycles();

  InvariantGraphApplyResult apply(propagation::SolverBase&);

 private:
  std::unordered_set<VarNodeId, VarNodeIdHash> dynamicVarNodeFrontier(
      const VarNodeId node,
      const std::unordered_set<VarNodeId, VarNodeIdHash>& visitedGlobal);

  VarNodeId findCycleUtil(
      const VarNodeId varNodeId,
      const std::unordered_set<VarNodeId, VarNodeIdHash>& visitedGlobal,
      std::unordered_set<VarNodeId, VarNodeIdHash>& visitedLocal,
      std::unordered_map<VarNodeId, VarNodeId, VarNodeIdHash>& path);

  std::vector<VarNodeId> findCycle(
      const std::unordered_map<VarNodeId, VarNodeId, VarNodeIdHash>& childOf,
      VarNodeId const node, VarNodeId const parent);

  std::pair<VarNodeId, InvariantNodeId> findPivotInCycle(
      const std::vector<VarNodeId>& cycle);

  std::vector<VarNodeId> breakCycles(
      VarNodeId node,
      std::unordered_set<VarNodeId, VarNodeIdHash>& visitedGlobal);
  VarNodeId breakCycle(const std::vector<VarNodeId>& cycle);

  void createVars(propagation::SolverBase&);
  void createImplicitConstraints(propagation::SolverBase&);
  void createInvariants(propagation::SolverBase&);
  propagation::VarId createViolations(propagation::SolverBase&);
};

}  // namespace atlantis::invariantgraph