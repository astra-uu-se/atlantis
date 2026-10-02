#pragma once

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/implicitConstraintNode.hpp"
#include "atlantis/invariantgraph/solverMapping.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::search::neighborhoods {
class NeighborhoodCombinator;
}

namespace atlantis::invariantgraph {
class InvariantGraphRoot;
class VarNode;
class InvariantNode;
class ImplicitConstraintNode;

class InvariantGraph {
  std::vector<std::shared_ptr<VarNode>> _varNodes;
  std::unordered_map<std::string, std::shared_ptr<VarNode>>
      _namedVarNodeIndices;
  std::unordered_map<Int, std::shared_ptr<VarNode>> _intVarNodeIndices;
  std::array<std::shared_ptr<VarNode>, 2> _boolVarNodes;
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
  std::shared_ptr<VarNode> _objectiveVarNode;
  ObjectiveDirection _objectiveDirection{ObjectiveDirection::NONE};

 public:
  explicit InvariantGraph(bool breakDynamicCycles = false);

  virtual ~InvariantGraph() = default;

  InvariantGraph(const InvariantGraph&) = delete;
  InvariantGraph(InvariantGraph&&) = default;

  [[nodiscard]] ConstraintSolver& constraintSolver();

  [[nodiscard]] const ConstraintSolver& constraintSolverConst() const;

  [[nodiscard]] virtual bool containsVarNode(const std::string&) const;

  [[nodiscard]] virtual bool containsVarNode(Int) const;

  [[nodiscard]] virtual bool containsVarNode(bool) const;

  virtual VarNode& retrieveBoolVarNode(DomainType);

  virtual VarNode& retrieveBoolVarNode() {
    return retrieveBoolVarNode(DomainType::DOM_RANGE);
  }

  virtual VarNode& retrieveBoolVarNode(const std::string&, DomainType);

  virtual VarNode& retrieveBoolVarNode(const std::string& identifier) {
    return retrieveBoolVarNode(identifier, DomainType::DOM_RANGE);
  }

  virtual VarNode& retrieveBoolVarNode(bool);

  virtual VarNode& retrieveBoolVarNode(bool, bool forceNewVar);

  virtual VarNode& retrieveBoolVarNode(bool, const std::string&);

  virtual VarNode& retrieveBoolVarNode(const std::shared_ptr<SearchDomain>&,
                                       DomainType);

  virtual VarNode& retrieveBoolVarNode(
      const std::shared_ptr<SearchDomain>& dom) {
    return retrieveBoolVarNode(dom, DomainType::DOM_RANGE);
  }

  virtual VarNode retrieveIntVarNode(const std::string&);

  virtual VarNode& retrieveIntVarNode(Int);

  virtual VarNode& retrieveIntVarNode(Int, bool forceNewVar);

  virtual VarNode& retrieveIntVarNode(Int, const std::string&);

  virtual VarNode& retrieveIntVarNode(const std::shared_ptr<SearchDomain>&,
                                      DomainType);

  virtual VarNode& retrieveIntVarNode(
      const std::shared_ptr<SearchDomain>& domain);

  virtual VarNode& retrieveIntVarNode(const std::shared_ptr<SearchDomain>&,
                                      const std::string&, DomainType);

  virtual VarNode& retrieveIntVarNode(const std::shared_ptr<SearchDomain>& dom,
                                      const std::string& identifier);

  void setObjective(const std::shared_ptr<VarNode>& objNode,
                    ObjectiveDirection objDir);

  [[nodiscard]] VarNode& varNode(const std::string& identifier);

  [[nodiscard]] VarNode& varNode(Int value);

  [[nodiscard]] const VarNode& varNodeConst(const std::string&) const;

  [[nodiscard]] const std::vector<std::shared_ptr<ImplicitConstraintNode>>&
  implicitConstraintNodes() const;

  std::shared_ptr<InvariantNode> addInvariantNode(
      std::shared_ptr<InvariantNode>&&);

  void updateDomains();

  /**
   * @brief replaces the given old VarNode with the new VarNode in
   * all Invariants.
   */
  void replaceVarNode(std::shared_ptr<VarNode> oldNode,
                      std::shared_ptr<VarNode> newNode);

  std::shared_ptr<ImplicitConstraintNode> addImplicitConstraintNode(
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
  void deactivateUnusedInvariantNodes();

  void makeImplicitConstraintNodes();

  [[nodiscard]] InvariantGraphRoot& root() const;

  void writeDotFile(std::ostream&) const;
};

}  // namespace atlantis::invariantgraph
