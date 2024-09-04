#pragma once

#include <array>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "atlantis/invariantgraph/iImplicitConstraintNode.hpp"
#include "atlantis/invariantgraph/iInvariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::invariantgraph {

class IInvariantGraph {
 public:
  virtual ~IInvariantGraph() = default;

  [[nodiscard]] virtual propagation::SolverBase& solver() = 0;

  [[nodiscard]] virtual const propagation::SolverBase& solverConst() const = 0;

  [[nodiscard]] virtual VarNodeId nextVarNodeId() const = 0;

  [[nodiscard]] virtual bool containsVarNode(
      const std::string& identifier) const = 0;

  [[nodiscard]] virtual bool containsVarNode(Int) const = 0;

  [[nodiscard]] virtual bool containsVarNode(bool) const = 0;

  [[nodiscard]] virtual VarNodeId retrieveBoolVarNode() = 0;

  [[nodiscard]] virtual VarNodeId retrieveBoolVarNode(VarNode::DomainType) = 0;

  [[nodiscard]] virtual VarNodeId retrieveBoolVarNode(const std::string&) = 0;

  [[nodiscard]] virtual VarNodeId retrieveBoolVarNode(const std::string&,
                                                      VarNode::DomainType) = 0;

  [[nodiscard]] virtual VarNodeId retrieveBoolVarNode(bool) = 0;

  [[nodiscard]] virtual VarNodeId retrieveBoolVarNode(bool,
                                                      const std::string&) = 0;

  [[nodiscard]] virtual VarNodeId retrieveBoolVarNode(SearchDomain&&) = 0;

  [[nodiscard]] virtual VarNodeId retrieveBoolVarNode(SearchDomain&&,
                                                      VarNode::DomainType) = 0;

  [[nodiscard]] virtual VarNodeId retrieveIntVarNode(const std::string&) = 0;

  [[nodiscard]] virtual VarNodeId retrieveIntVarNode(Int) = 0;

  [[nodiscard]] virtual VarNodeId retrieveIntVarNode(Int,
                                                     const std::string&) = 0;

  [[nodiscard]] virtual VarNodeId retrieveIntVarNode(SearchDomain&&) = 0;

  [[nodiscard]] virtual VarNodeId retrieveIntVarNode(SearchDomain&&,
                                                     VarNode::DomainType) = 0;

  [[nodiscard]] virtual VarNodeId retrieveIntVarNode(SearchDomain&&,
                                                     const std::string&) = 0;

  [[nodiscard]] virtual VarNodeId retrieveIntVarNode(SearchDomain&&,
                                                     const std::string&,
                                                     VarNode::DomainType) = 0;

  [[nodiscard]] virtual VarNode& varNode(const std::string&) = 0;

  [[nodiscard]] virtual VarNode& varNode(VarNodeId) = 0;

  [[nodiscard]] virtual VarNode& varNode(Int) = 0;

  [[nodiscard]] virtual const VarNode& varNodeConst(
      const std::string&) const = 0;

  [[nodiscard]] virtual const VarNode& varNodeConst(VarNodeId) const = 0;

  [[nodiscard]] virtual VarNodeId varNodeId(bool) const = 0;

  [[nodiscard]] virtual VarNodeId varNodeId(Int) const = 0;

  [[nodiscard]] virtual VarNodeId varNodeId(const std::string&) const = 0;

  [[nodiscard]] virtual propagation::VarId varId(
      const std::string& identifier) const = 0;
  [[nodiscard]] virtual propagation::VarId varId(VarNodeId id) const = 0;

  [[nodiscard]] virtual bool containsInvariantNode(InvariantNodeId) const = 0;
  [[nodiscard]] virtual bool containsImplicitConstraintNode(
      InvariantNodeId) const = 0;

  [[nodiscard]] virtual IInvariantNode& invariantNode(InvariantNodeId) = 0;
  [[nodiscard]] virtual IImplicitConstraintNode& implicitConstraintNode(
      InvariantNodeId) = 0;

  [[nodiscard]] virtual InvariantNodeId nextInvariantNodeId() const = 0;
  [[nodiscard]] virtual InvariantNodeId nextImplicitNodeId() const = 0;

  InvariantNodeId virtual addInvariantNode(
      std::shared_ptr<IInvariantNode>&&) = 0;

  /**
   * @brief replaces the given old VarNode with the new VarNode in
   * all Invariants.
   */
  virtual void replaceVarNode(VarNodeId oldNodeId, VarNodeId newNodeId) = 0;

  virtual InvariantNodeId addImplicitConstraintNode(
      std::shared_ptr<IImplicitConstraintNode>&&) = 0;

  [[nodiscard]] virtual propagation::VarId totalViolationVarId() const = 0;

  [[nodiscard]] virtual const VarNode& objectiveVarNode() const = 0;

  [[nodiscard]] virtual propagation::VarId objectiveVarId() const = 0;

  virtual void construct() = 0;

  virtual void breakCycles() = 0;

  virtual void close() = 0;
};

}  // namespace atlantis::invariantgraph
