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
  struct VarNodeData {
    VarNode varNode;
    // If a variable is defined by muktiple invariants, then this is
    // the index in the _duplicateVarNodes vector with all the duplicates.
    // Otherwise, it is -1.
    Int duplicationIndex;
  };
  std::vector<VarNodeData> _varNodes;
  std::unordered_map<std::string, VarNodeId> _namedVarNodeIndices;
  std::unordered_map<Int, VarNodeId> _intVarNodeIndices;
  std::array<VarNodeId, 2> _boolVarNodeIndices;

  std::vector<std::unique_ptr<InvariantNode>> _invariantNodes;
  std::vector<std::unique_ptr<ImplicitConstraintNode>> _implicitConstraintNodes;

  std::vector<std::vector<VarNodeId>> _duplicateVarNodes;

  void populateRootNode();

  VarNodeData& varNodeData(VarNodeId id);
  VarNodeData& varNodeData(const std::string& identifier);

 protected:
  propagation::VarId _totalViolationVarId;
  VarNodeId _objectiveVarNodeId;
  Int markDuplicate(VarNodeId, const std::string&);

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

  VarNodeId inputBoolVarNode(bool);
  VarNodeId inputBoolVarNode(const std::string&);

  VarNodeId defineBoolVarNode();
  VarNodeId defineBoolVarNode(const std::string&);
  VarNodeId defineBoolVarNode(bool);
  VarNodeId defineBoolVarNode(bool, const std::string&);
  VarNodeId defineBoolVarNode(SearchDomain&&);

  VarNodeId inputIntVarNode(Int);
  VarNodeId inputIntVarNode(const std::string&);

  VarNodeId defineIntVarNode();
  VarNodeId defineIntVarNode(Int);
  VarNodeId defineIntVarNode(Int, const std::string&);
  VarNodeId defineIntVarNode(SearchDomain&&);
  VarNodeId defineIntVarNode(SearchDomain&&, const std::string&);

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

  [[nodiscard]] bool containsInvariantNode(InvariantNodeId) const noexcept;
  [[nodiscard]] bool containsImplicitConstraintNode(
      InvariantNodeId) const noexcept;

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