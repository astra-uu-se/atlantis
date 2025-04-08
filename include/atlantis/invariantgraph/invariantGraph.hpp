#pragma once

#include <array>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis {
class SearchDomain;
}

namespace atlantis::propagation {
class SolverBase;
}

namespace atlantis::search::neighborhoods {
class NeighborhoodCombinator;
}

namespace atlantis::invariantgraph {
class InvariantGraphRoot;
class VarNode;
class InvariantNode;
class ImplicitConstraintNode;

class InvariantGraph {
  propagation::SolverBase& _solver;
  std::vector<VarNode> _varNodes;
  std::unordered_map<std::string, VarNodeId> _namedVarNodeIndices;
  std::unordered_map<Int, VarNodeId> _intVarNodeIndices;
  std::array<VarNodeId, 2> _boolVarNodeIndices;

  std::vector<std::shared_ptr<InvariantNode>> _invariantNodes;
  std::vector<std::shared_ptr<ImplicitConstraintNode>>
      _implicitConstraintNodes;
  bool _breakDynamicCycles;

  void populateRootNode();

 protected:
  propagation::VarViewId _totalViolationVarId{propagation::NULL_ID};
  VarNodeId _objectiveVarNodeId;

 public:
  explicit InvariantGraph(propagation::SolverBase& solver,
                          bool breakDynamicCycles = false);
  virtual ~InvariantGraph() = default;

  InvariantGraph(const InvariantGraph&) = delete;
  InvariantGraph(InvariantGraph&&) = default;

  [[nodiscard]] virtual propagation::SolverBase& solver();

  [[nodiscard]] virtual const propagation::SolverBase& solverConst() const;

  [[nodiscard]] virtual VarNodeId nextVarNodeId() const;

  [[nodiscard]] virtual bool containsVarNode(const std::string&) const;

  [[nodiscard]] virtual bool containsVarNode(Int) const;

  [[nodiscard]] virtual bool containsVarNode(bool) const;

  virtual VarNodeId retrieveBoolVarNode(DomainType) ;

  virtual VarNodeId retrieveBoolVarNode() {
    return retrieveBoolVarNode(DomainType::DOM_RANGE);
  }

  virtual VarNodeId retrieveBoolVarNode(const std::string&, DomainType) ;

  virtual VarNodeId retrieveBoolVarNode(const std::string& identifier) {
    return retrieveBoolVarNode(identifier, DomainType::DOM_RANGE);
  }

  virtual VarNodeId retrieveBoolVarNode(bool);

  virtual VarNodeId retrieveBoolVarNode(bool, const std::string&);

  virtual VarNodeId retrieveBoolVarNode(const std::shared_ptr<SearchDomain>&,
                                DomainType);

  virtual VarNodeId retrieveBoolVarNode(
      const std::shared_ptr<SearchDomain>& dom) {
    return retrieveBoolVarNode(dom, DomainType::DOM_RANGE);
  }

  virtual VarNodeId retrieveIntVarNode(const std::string&);

  virtual VarNodeId retrieveIntVarNode(Int);

  virtual VarNodeId retrieveIntVarNode(Int, const std::string&);

  virtual VarNodeId retrieveIntVarNode(const std::shared_ptr<SearchDomain>&,
                               DomainType);

  virtual VarNodeId retrieveIntVarNode(
      const std::shared_ptr<SearchDomain>& domain);

  virtual VarNodeId retrieveIntVarNode(const std::shared_ptr<SearchDomain>&,
                               const std::string&, DomainType);

  virtual VarNodeId retrieveIntVarNode(const std::shared_ptr<SearchDomain>& dom,
                               const std::string& identifier);

  [[nodiscard]] VarNode& varNode(const std::string& identifier);

  [[nodiscard]] VarNode& varNode(VarNodeId id);

  [[nodiscard]] VarNode& varNode(Int value);

  [[nodiscard]] const VarNode& varNodeConst(const std::string&) const;

  [[nodiscard]] const VarNode& varNodeConst(VarNodeId id) const;

  [[nodiscard]] VarNodeId varNodeId(bool val) const;

  [[nodiscard]] VarNodeId varNodeId(Int val) const;

  [[nodiscard]] VarNodeId varNodeId(
      const std::string& identifier) const;

  [[nodiscard]] propagation::VarViewId varId(
      const std::string& identifier) const;

  [[nodiscard]] propagation::VarViewId varId(VarNodeId id) const;

  [[nodiscard]] bool containsInvariantNode(InvariantNodeId) const;

  [[nodiscard]] bool containsImplicitConstraintNode(
      InvariantNodeId) const;

  [[nodiscard]] InvariantNode& invariantNode(InvariantNodeId);

  [[nodiscard]] ImplicitConstraintNode& implicitConstraintNode(
      InvariantNodeId);

  [[nodiscard]] InvariantNodeId nextInvariantNodeId() const;

  [[nodiscard]] InvariantNodeId nextImplicitNodeId() const;

  InvariantNodeId addInvariantNode(std::shared_ptr<InvariantNode>&&);

  /**
   * @brief replaces the given old VarNode with the new VarNode in
   * all Invariants.
   */
  void replaceVarNode(VarNodeId oldNodeId, VarNodeId newNodeId);

  InvariantNodeId addImplicitConstraintNode(
      std::shared_ptr<ImplicitConstraintNode>&&);

  [[nodiscard]] propagation::VarViewId totalViolationVarId() const;

  [[nodiscard]] const VarNode& objectiveVarNode() const;

  [[nodiscard]] propagation::VarViewId objectiveVarId() const;

  void breakCycles();

  void construct();

  void close();

  void splitMultiDefinedVars();

  void replaceFixedVars();

  void replaceInvariantNodes();

  [[nodiscard]] InvariantGraphRoot& root() const;

  [[nodiscard]] search::neighborhoods::NeighborhoodCombinator neighborhood()
      const;

  void writeDotFile(std::ostream&) const;

 private:
  std::unordered_set<VarNodeId> dynamicVarNodeFrontier(
      VarNodeId node, const std::unordered_set<VarNodeId>& visitedGlobal);

  VarNodeId findCycleUtil(
      VarNodeId varNodeId, const std::unordered_set<VarNodeId>& visitedGlobal,
      std::unordered_set<VarNodeId>& visitedLocal,
      std::unordered_map<VarNodeId, InvariantGraphEdge>& path);

  InvariantGraphEdge findPivotInCycle(
      const std::vector<InvariantGraphEdge>& cycle);

  void breakSelfCycles();

  std::vector<VarNodeId> breakCycles(
      VarNodeId node, std::unordered_set<VarNodeId>& visitedGlobal);
  VarNodeId breakCycle(const std::vector<InvariantGraphEdge>& cycle);

  void createVars();
  void createImplicitConstraints();
  void createInvariants();
  propagation::VarViewId createViolations();
  void sanity(bool);
};

}  // namespace atlantis::invariantgraph
