#pragma once

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>
#include "atlantis/utils/gecode_compat.hpp"
#include <gecode/kernel.hh>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/solverMapping.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"
#include "implicitConstraintNode.hpp"

namespace atlantis {
class SearchDomain;
}

namespace atlantis::search::neighborhoods {
class NeighborhoodCombinator;
}

namespace atlantis::invariantgraph {
class ConstraintSolver;
class InvariantGraphRoot;
class VarNode;
class InvariantNode;
class ImplicitConstraintNode;

class InvariantGraph {
  std::vector<VarNode> _varNodes;
  std::unordered_map<std::string, VarNodeId> _namedVarNodeIndices;
  std::unordered_map<Int, VarNodeId> _intVarNodeIndices;
  std::array<VarNodeId, 2> _boolVarNodeIndices;
  std::shared_ptr<ConstraintSolver> _constraintSolver;

  std::vector<std::shared_ptr<InvariantNode>> _invariantNodes;
  std::vector<std::shared_ptr<ImplicitConstraintNode>> _implicitConstraintNodes;

  bool _breakDynamicCycles;
  bool _isOpen{false};

  void populateRootNode();

  void breakSelfCycles();

  void createVars(propagation::SolverBase&, SolverMapping&) const;
  void createImplicitConstraints(propagation::SolverBase&,
                                 SolverMapping&) const;
  void createInvariants(propagation::SolverBase&, SolverMapping&) const;
  void createNeighborhood(propagation::SolverBase&, SolverMapping&) const;

  propagation::VarViewId createViolations(propagation::SolverBase&,
                                          SolverMapping&) const;
  void sanity(bool);

 protected:
  VarNodeId _objectiveVarNodeId;
  ObjectiveDirection _objectiveDirection{ObjectiveDirection::NONE};

 public:
  explicit InvariantGraph(bool breakDynamicCycles = false);

  virtual ~InvariantGraph() = default;

  InvariantGraph(const InvariantGraph&) = delete;
  InvariantGraph(InvariantGraph&&) = default;

  [[nodiscard]] virtual VarNodeId nextVarNodeId() const;

  [[nodiscard]] virtual bool containsVarNode(const std::string&) const;

  [[nodiscard]] virtual bool containsVarNode(Int) const;

  [[nodiscard]] virtual bool containsVarNode(bool) const;

  virtual VarNodeId retrieveBoolVarNode(DomainType);

  virtual VarNodeId retrieveBoolVarNode() {
    return retrieveBoolVarNode(DomainType::DOM_RANGE);
  }

  virtual VarNodeId retrieveBoolVarNode(const std::string&, DomainType);

  virtual VarNodeId retrieveBoolVarNode(const std::string& identifier) {
    return retrieveBoolVarNode(identifier, DomainType::DOM_RANGE);
  }

  virtual VarNodeId retrieveBoolVarNode(bool);

  virtual VarNodeId retrieveBoolVarNode(bool, bool forceNewVar);

  virtual VarNodeId retrieveBoolVarNode(bool, const std::string&);

  virtual VarNodeId retrieveBoolVarNode(const std::shared_ptr<SearchDomain>&,
                                        DomainType);

  virtual VarNodeId retrieveBoolVarNode(
      const std::shared_ptr<SearchDomain>& dom) {
    return retrieveBoolVarNode(dom, DomainType::DOM_RANGE);
  }

  virtual VarNodeId retrieveIntVarNode(const std::string&);

  virtual VarNodeId retrieveIntVarNode(Int);

  virtual VarNodeId retrieveIntVarNode(Int, bool forceNewVar);

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

  [[nodiscard]] const InvariantNode& invariantNodeConst(InvariantNodeId) const;

  [[nodiscard]] const std::vector<std::shared_ptr<ImplicitConstraintNode>>&
  implicitConstraintNodes() const;

  [[nodiscard]] VarNodeId varNodeId(bool val) const;

  [[nodiscard]] VarNodeId varNodeId(Int val) const;

  [[nodiscard]] VarNodeId varNodeId(const std::string& identifier) const;

  [[nodiscard]] bool containsInvariantNode(InvariantNodeId) const;

  [[nodiscard]] bool containsImplicitConstraintNode(InvariantNodeId) const;

  [[nodiscard]] InvariantNode& invariantNode(InvariantNodeId);

  [[nodiscard]] ImplicitConstraintNode& implicitConstraintNode(InvariantNodeId);

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

  [[nodiscard]] const VarNode& objectiveVarNode() const;

  void breakCycles();

  void open();

  [[nodiscard]] bool isOpen() const { return _isOpen; };

  void close();

  SolverMapping construct(propagation::SolverBase&) const;

  void splitMultiDefinedVars();

  void replaceFixedVars();

  void replaceInvariantNodes();

  [[nodiscard]] InvariantGraphRoot& root() const;

  void writeDotFile(std::ostream&) const;
};

}  // namespace atlantis::invariantgraph
