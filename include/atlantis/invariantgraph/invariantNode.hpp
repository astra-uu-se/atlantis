#pragma once

#include <vector>

#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {
class SolverBase;  //  forward declaration;
}

namespace atlantis::invariantgraph {
class ConstraintSolver;

class InvariantGraph;  // forward declaration;

/**
 * A node in the invariant graph which defines a number of variables. This could
 * be an invariant, a violation invariant (which defines a violation), or a
 * view.
 */

class InvariantNode : public std::enable_shared_from_this<InvariantNode> {
  InvariantNodeState _state{InvariantNodeState::UNINITIALIZED};
  InvariantGraph& _invariantGraph;
  InvariantNodeId _mappingId{NULL_NODE_ID};

 protected:
  std::vector<std::shared_ptr<VarNode>> _outputVarNodes;
  std::vector<std::shared_ptr<VarNode>> _staticInputVarNodes;
  std::vector<std::shared_ptr<VarNode>> _dynamicInputVarNodes;

  explicit InvariantNode(InvariantGraph& invariantGraph,
                         std::vector<std::shared_ptr<VarNode>>&& outputIds,
                         std::vector<std::shared_ptr<VarNode>>&& staticInputIds = {},
                         std::vector<std::shared_ptr<VarNode>>&& dynamicInputIds = {});



 public:
  std::shared_ptr<InvariantNode> getPtr();

  virtual ~InvariantNode() = default;

  [[nodiscard]] InvariantGraph& invariantGraph();

  [[nodiscard]] ConstraintSolver& constraintSolver() const;

  [[nodiscard]] const InvariantGraph& invariantGraphConst() const;

  [[nodiscard]] const ConstraintSolver& constraintSolverConst() const;

  virtual void init();

  void setMappingId(InvariantNodeId);

  InvariantNodeId mappingId() const;

  virtual void postConstraint();

  [[nodiscard]] virtual bool isReified() const;

  [[nodiscard]] virtual bool isViolationInvariant() const;

  virtual void updateState();

  [[nodiscard]] virtual bool constrainsOutput(
      const VarNode& outputVarNode) const = 0;

  [[nodiscard]] virtual bool canBeReplaced() const;

  [[nodiscard]] virtual bool replace();

  [[nodiscard]] virtual std::pair<size_t, size_t> implicitRank() const;

  [[nodiscard]] virtual bool canBeMadeImplicit() const;

  [[nodiscard]] virtual bool makeImplicit();

  [[nodiscard]] InvariantNodeState state() const;

  /**
   * @return The violation variable of this variable defining node. Only
   * applicable if the current node is a violation invariant. If this node does
   * not define a violation variable, this method returns propagation::NULL_ID.
   */
  [[nodiscard]] virtual propagation::VarViewId violationVarId(
      const SolverMapping&) const;

  /**
   * @return The variable nodes defined by this node.
   */
  [[nodiscard]] const std::vector<std::shared_ptr<VarNode>>& outputVarNodes() const;

  [[nodiscard]] const std::vector<std::shared_ptr<VarNode>>& staticInputVarNodes() const;

  [[nodiscard]] const std::vector<std::shared_ptr<VarNode>>& dynamicInputVarNodes() const;

  void setState(InvariantNodeState);

  void deactivate();

  void replaceDefinedVar(VarNode& oldOutputVarNode,
                         const std::shared_ptr<VarNode>& newOutputVarNode);

  void removeStaticInputVarNode(VarNode&);

  void removeStaticInputAtIndex(size_t);

  void removeDynamicInputVarNode(VarNode&);

  void removeDynamicInputAtIndex(size_t);

  virtual void removeOutputVarNode(VarNode&);

  virtual void removeOutputAtIndex(size_t);

  void eraseStaticInputVarNode(size_t index);

  void eraseDynamicInputVarNode(size_t index);

  void replaceStaticInputVarNode(VarNode& oldStaticVar,
                                 const std::shared_ptr<VarNode>& newStaticVar);

  void replaceDynamicInputVarNode(VarNode& oldDynamicVar,
                                  const std::shared_ptr<VarNode>& newDynamicVar);

  [[nodiscard]] std::vector<std::pair<std::shared_ptr<VarNode>, std::shared_ptr<VarNode>>>
  splitOutputVarNodes();

  propagation::VarViewId makeSolverVar(const VarNode& varNode,
                                       propagation::SolverBase&,
                                       SolverMapping&) const;

  propagation::VarViewId makeSolverVar(const VarNode& varNode, Int initialValue,
                                       propagation::SolverBase&,
                                       SolverMapping&) const;

  void markOutputTo(const std::shared_ptr<VarNode>& varNode, bool registerHere);

  void markStaticInputTo(const std::shared_ptr<VarNode>& varNode, bool registerHere);

  void markDynamicInputTo(const std::shared_ptr<VarNode>& varNode, bool registerHere);

  virtual void registerOutputVars(propagation::SolverBase&,
                                  SolverMapping&) const = 0;

  virtual void registerNode(propagation::SolverBase&, SolverMapping&) const = 0;

  [[nodiscard]] virtual std::string dotLangIdentifier() const = 0;

  virtual std::ostream& dotLangEdges(std::ostream&) const;

  virtual std::ostream& dotLangEntry(std::ostream&) const;
};

}  // namespace atlantis::invariantgraph
