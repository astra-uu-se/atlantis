#pragma once

#include <array>
#include <unordered_map>
#include <unordered_set>

#include "atlantis/invariantgraph/iInvariantGraph.hpp"
#include "atlantis/invariantgraph/invariantGraphRoot.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/search/neighbourhoods/neighbourhoodCombinator.hpp"

namespace atlantis::invariantgraph {

class InvariantGraph : public virtual IInvariantGraph {
 private:
  propagation::SolverBase& _solver;
  std::vector<VarNode> _varNodes;
  std::unordered_map<std::string, VarNodeId> _namedVarNodeIndices;
  std::unordered_map<Int, VarNodeId> _intVarNodeIndices;
  std::array<VarNodeId, 2> _boolVarNodeIndices;

  std::vector<std::shared_ptr<IInvariantNode>> _invariantNodes;
  std::vector<std::shared_ptr<IImplicitConstraintNode>>
      _implicitConstraintNodes;
  bool _breakDynamicCycles;

  void populateRootNode();

 protected:
  propagation::VarViewId _totalViolationVarId{propagation::NULL_ID};
  VarNodeId _objectiveVarNodeId;

 public:
  InvariantGraph(propagation::SolverBase& solver,
                 bool breakDynamicCycles = false);
  virtual ~InvariantGraph() = default;

  InvariantGraph(const InvariantGraph&) = delete;
  InvariantGraph(InvariantGraph&&) = default;

  [[nodiscard]] propagation::SolverBase& solver() override;

  [[nodiscard]] const propagation::SolverBase& solverConst() const override;

  [[nodiscard]] VarNodeId nextVarNodeId() const override;

  [[nodiscard]] bool containsVarNode(const std::string&) const override;

  [[nodiscard]] bool containsVarNode(Int) const override;

  [[nodiscard]] bool containsVarNode(bool) const override;

  VarNodeId retrieveBoolVarNode(VarNode::DomainType) override;

  VarNodeId retrieveBoolVarNode() override {
    return retrieveBoolVarNode(VarNode::DomainType::RANGE);
  }

  VarNodeId retrieveBoolVarNode(const std::string&,
                                VarNode::DomainType) override;

  VarNodeId retrieveBoolVarNode(const std::string& identifier) override {
    return retrieveBoolVarNode(identifier, VarNode::DomainType::RANGE);
  }

  VarNodeId retrieveBoolVarNode(bool) override;

  VarNodeId retrieveBoolVarNode(bool, const std::string&) override;

  VarNodeId retrieveBoolVarNode(SearchDomain&&, VarNode::DomainType) override;

  VarNodeId retrieveBoolVarNode(SearchDomain&& dom) override {
    return retrieveBoolVarNode(std::move(dom), VarNode::DomainType::RANGE);
  }

  VarNodeId retrieveIntVarNode(const std::string&) override;

  VarNodeId retrieveIntVarNode(Int) override;

  VarNodeId retrieveIntVarNode(Int, const std::string&) override;

  VarNodeId retrieveIntVarNode(SearchDomain&&, VarNode::DomainType) override;

  VarNodeId retrieveIntVarNode(SearchDomain&& dom) override {
    return retrieveIntVarNode(std::move(dom), VarNode::DomainType::DOMAIN);
  }

  VarNodeId retrieveIntVarNode(SearchDomain&& dom, const std::string& str,
                               VarNode::DomainType) override;

  VarNodeId retrieveIntVarNode(SearchDomain&& dom,
                               const std::string& identifier) override {
    return retrieveIntVarNode(std::move(dom), identifier,
                              VarNode::DomainType::DOMAIN);
  }

  [[nodiscard]] VarNode& varNode(const std::string& identifier) override;

  [[nodiscard]] VarNode& varNode(VarNodeId id) override;

  [[nodiscard]] VarNode& varNode(Int value) override;

  [[nodiscard]] const VarNode& varNodeConst(const std::string&) const override;

  [[nodiscard]] const VarNode& varNodeConst(VarNodeId id) const override;

  [[nodiscard]] VarNodeId varNodeId(bool val) const override;

  [[nodiscard]] VarNodeId varNodeId(Int val) const override;

  [[nodiscard]] VarNodeId varNodeId(
      const std::string& identifier) const override;

  [[nodiscard]] propagation::VarViewId varId(
      const std::string& identifier) const override;

  [[nodiscard]] propagation::VarViewId varId(VarNodeId id) const override;

  [[nodiscard]] bool containsInvariantNode(InvariantNodeId) const override;

  [[nodiscard]] bool containsImplicitConstraintNode(
      InvariantNodeId) const override;

  [[nodiscard]] IInvariantNode& invariantNode(InvariantNodeId) override;

  [[nodiscard]] IImplicitConstraintNode& implicitConstraintNode(
      InvariantNodeId) override;

  [[nodiscard]] InvariantNodeId nextInvariantNodeId() const override;

  [[nodiscard]] InvariantNodeId nextImplicitNodeId() const override;

  InvariantNodeId addInvariantNode(std::shared_ptr<IInvariantNode>&&) override;

  /**
   * @brief replaces the given old VarNode with the new VarNode in
   * all Invariants.
   */
  void replaceVarNode(VarNodeId oldNodeId, VarNodeId newNodeId) override;

  InvariantNodeId addImplicitConstraintNode(
      std::shared_ptr<IImplicitConstraintNode>&&) override;

  [[nodiscard]] propagation::VarViewId totalViolationVarId() const override;

  [[nodiscard]] const VarNode& objectiveVarNode() const override;

  [[nodiscard]] propagation::VarViewId objectiveVarId() const override;

  void breakCycles() override;

  void construct() override;

  void close() override;

  void splitMultiDefinedVars();

  void replaceFixedVars();

  void replaceInvariantNodes();

  [[nodiscard]] InvariantGraphRoot& root();

  [[nodiscard]] search::neighbourhoods::NeighbourhoodCombinator neighbourhood()
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
