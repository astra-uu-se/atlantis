#pragma once

#include <array>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "atlantis/invariantgraph/implicitConstraintNode.hpp"
#include "atlantis/invariantgraph/invariantGraphRoot.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/search/neighbourhoods/neighbourhoodCombinator.hpp"

namespace atlantis::invariantgraph {

class InvariantGraph {
 private:
  std::vector<VarNode> _varNodes;
  std::unordered_map<std::string, VarNodeId> _namedVarNodeIndices;
  std::unordered_map<Int, VarNodeId> _intVarNodeIndices;
  std::array<VarNodeId, 2> _boolVarNodeIndices;

  std::vector<std::unique_ptr<InvariantNode>> _invariantNodes;
  std::vector<std::unique_ptr<ImplicitConstraintNode>> _implicitConstraintNodes;

  void populateRootNode();

 protected:
  propagation::VarId _totalViolationVarId;
  VarNodeId _objectiveVarNodeId;

 public:
  InvariantGraph();
  virtual ~InvariantGraph() = default;

  InvariantGraph(const InvariantGraph&) = delete;
  InvariantGraph(InvariantGraph&&) = default;

  [[nodiscard]] VarNodeId nextVarNodeId() const noexcept;

  [[nodiscard]] bool containsVarNode(
      const std::string& identifier) const noexcept;
  [[nodiscard]] bool containsVarNode(Int) const noexcept;
  [[nodiscard]] static bool containsVarNode(bool) noexcept;

  VarNodeId retrieveBoolVarNode(
      VarNode::DomainType = VarNode::DomainType::RANGE);
  VarNodeId retrieveBoolVarNode(
      const std::string&, VarNode::DomainType = VarNode::DomainType::RANGE);
  VarNodeId retrieveBoolVarNode(bool);
  VarNodeId retrieveBoolVarNode(bool, const std::string&);
  VarNodeId retrieveBoolVarNode(
      SearchDomain&&, VarNode::DomainType = VarNode::DomainType::RANGE);

  VarNodeId retrieveIntVarNode(const std::string&);
  VarNodeId retrieveIntVarNode(Int);
  VarNodeId retrieveIntVarNode(Int, const std::string&);
  VarNodeId retrieveIntVarNode(
      SearchDomain&&, VarNode::DomainType = VarNode::DomainType::DOMAIN);
  VarNodeId retrieveIntVarNode(
      SearchDomain&&, const std::string&,
      VarNode::DomainType = VarNode::DomainType::DOMAIN);

  [[nodiscard]] VarNode& varNode(const std::string& identifier);
  [[nodiscard]] VarNode& varNode(VarNodeId id);
  [[nodiscard]] VarNode& varNode(Int value);

  [[nodiscard]] const VarNode& varNodeConst(
      const std::string& identifier) const;
  [[nodiscard]] const VarNode& varNodeConst(VarNodeId id) const;

  [[nodiscard]] VarNodeId varNodeId(bool val) const;
  [[nodiscard]] VarNodeId varNodeId(Int val) const;
  [[nodiscard]] VarNodeId varNodeId(const std::string& identifier) const;

  [[nodiscard]] propagation::VarId varId(const std::string& identifier) const;
  [[nodiscard]] propagation::VarId varId(VarNodeId id) const;

  [[nodiscard]] bool containsInvariantNode(InvariantNodeId) const noexcept;
  [[nodiscard]] bool containsImplicitConstraintNode(
      InvariantNodeId) const noexcept;

  [[nodiscard]] InvariantNode& invariantNode(InvariantNodeId);
  [[nodiscard]] InvariantGraphRoot& root();
  [[nodiscard]] ImplicitConstraintNode& implicitConstraintNode(InvariantNodeId);

  [[nodiscard]] InvariantNodeId nextInvariantNodeId() const noexcept;
  [[nodiscard]] InvariantNodeId nextImplicitNodeId() const noexcept;

  InvariantNodeId addInvariantNode(std::unique_ptr<InvariantNode>&&);

  /**
   * @brief replaces the given old VarNode with the new VarNode in
   * all Invariants.
   */
  void replaceVarNode(VarNodeId oldNodeId, VarNodeId newNodeId);

  void replaceInvariantNodes();

  InvariantNodeId addImplicitConstraintNode(
      std::unique_ptr<ImplicitConstraintNode>&&);

  [[nodiscard]] search::neighbourhoods::NeighbourhoodCombinator neighbourhood()
      const noexcept;

  [[nodiscard]] propagation::VarId totalViolationVarId() const noexcept;

  [[nodiscard]] propagation::VarId objectiveVarId() const noexcept;

  void splitMultiDefinedVars();
  void breakCycles();

  void apply(propagation::SolverBase&);

 private:
  std::unordered_set<VarNodeId, VarNodeIdHash> dynamicVarNodeFrontier(
      VarNodeId node,
      const std::unordered_set<VarNodeId, VarNodeIdHash>& visitedGlobal);

  VarNodeId findCycleUtil(
      VarNodeId varNodeId,
      const std::unordered_set<VarNodeId, VarNodeIdHash>& visitedGlobal,
      std::unordered_set<VarNodeId, VarNodeIdHash>& visitedLocal,
      std::unordered_map<VarNodeId, VarNodeId, VarNodeIdHash>& path);

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
