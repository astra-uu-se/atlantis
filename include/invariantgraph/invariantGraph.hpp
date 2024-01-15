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
#include "propagation/types.hpp"
#include "search/neighbourhoods/neighbourhoodCombinator.hpp"
#include "utils/fznAst.hpp"

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

  [[nodiscard]] bool containsVarNode(
      const std::string& identifier) const noexcept;
  [[nodiscard]] bool containsVarNode(Int) const noexcept;
  [[nodiscard]] bool containsVarNode(bool) const noexcept;

  virtual VarNodeId createVarNode(bool, bool isDefinedVar);
  virtual VarNodeId createVarNode(bool, const std::string&, bool isDefinedVar);
  virtual VarNodeId createVarNode(Int, bool isDefinedVar);
  virtual VarNodeId createVarNode(Int, const std::string&, bool isDefinedVar);
  virtual VarNodeId createVarNode(const SearchDomain&, bool isIntVar,
                                  bool isDefinedVar);
  virtual VarNodeId createVarNode(const SearchDomain&, bool isIntVar,
                                  const std::string&, bool isDefinedVar);
  virtual VarNodeId createVarNode(const VarNode&, bool isDefinedVar);

  [[nodiscard]] VarNode& varNode(const std::string& identifier);
  [[nodiscard]] VarNode& varNode(VarNodeId id);

  [[nodiscard]] const VarNode& varNodeConst(
      const std::string& identifier) const;
  [[nodiscard]] const VarNode& varNodeConst(VarNodeId id) const;

  [[nodiscard]] VarNodeId varNodeId(bool val) const;
  [[nodiscard]] VarNodeId varNodeId(Int val) const;
  [[nodiscard]] VarNodeId varNodeId(const std::string& identifier) const;

  [[nodiscard]] propagation::VarId varId(const std::string& identifier) const;
  [[nodiscard]] propagation::VarId varId(VarNodeId id) const;

  [[nodiscard]] InvariantNode& invariantNode(InvariantNodeId);
  [[nodiscard]] InvariantGraphRoot& root();
  [[nodiscard]] ImplicitConstraintNode& implicitConstraintNode(InvariantNodeId);

  [[nodiscard]] InvariantNodeId nextInvariantNodeId() const noexcept;
  [[nodiscard]] InvariantNodeId nextImplicitNodeId() const noexcept;

  InvariantNodeId addInvariantNode(std::unique_ptr<InvariantNode>&&);

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