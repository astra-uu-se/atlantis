#pragma once

#include <vector>

#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/types.hpp"
#include "solverMapping.hpp"

namespace atlantis::propagation {
class SolverBase;  //  forward declaration;
}

namespace atlantis::invariantgraph {

class InvariantGraph;  // forward declaration;

/**
 * A node in the invariant graph which defines a number of variables. This could
 * be an invariant, a violation invariant (which defines a violation), or a
 * view.
 */

class InvariantNode {
  InvariantNodeId _id{NULL_NODE_ID};
  InvariantNodeState _state{InvariantNodeState::UNINITIALIZED};
  InvariantGraph& _invariantGraph;

 protected:
  std::vector<VarNodeId> _outputVarNodeIds;
  std::vector<VarNodeId> _staticInputVarNodeIds;
  std::vector<VarNodeId> _dynamicInputVarNodeIds;

 public:
  explicit InvariantNode(InvariantGraph& invariantGraph,
                         std::vector<VarNodeId>&& outputIds,
                         std::vector<VarNodeId>&& staticInputIds = {},
                         std::vector<VarNodeId>&& dynamicInputIds = {});

  virtual ~InvariantNode() = default;

  [[nodiscard]] InvariantGraph& invariantGraph();

  [[nodiscard]] const InvariantGraph& invariantGraphConst() const;

  [[nodiscard]] InvariantNodeId id() const;

  [[nodiscard]] virtual bool isReified() const;

  virtual void updateState();

  [[nodiscard]] virtual bool canBeReplaced() const;

  [[nodiscard]] virtual bool replace();

  [[nodiscard]] virtual bool canBeMadeImplicit() const;

  [[nodiscard]] virtual bool makeImplicit();

  [[nodiscard]] InvariantNodeState state() const;

  /**
   * @return The violation variable of this variable defining node. Only
   * applicable if the current node is a violation invariant. If this node does
   * not define a violation variable, this method returns propagation::NULL_ID.
   */
  [[nodiscard]] virtual propagation::VarViewId violationVarId(const SolverMapping&) const;

  /**
   * @return The variable nodes defined by this node.
   */
  [[nodiscard]] const std::vector<VarNodeId>& outputVarNodeIds() const;

  [[nodiscard]] const std::vector<VarNodeId>& staticInputVarNodeIds() const;

  [[nodiscard]] const std::vector<VarNodeId>& dynamicInputVarNodeIds() const;

  void setState(InvariantNodeState);

  virtual void init(InvariantNodeId);

  void deactivate();

  void replaceDefinedVar(VarNodeId oldOutputVarNodeId,
                         VarNodeId newOutputVarNodeId);

  void removeStaticInputVarNode(VarNodeId);

  void removeStaticInputAtIndex(size_t);

  void removeDynamicInputVarNode(VarNodeId);

  void removeDynamicInputAtIndex(size_t);

  void removeOutputVarNode(VarNodeId);

  void removeOutputAtIndex(size_t);

  void eraseStaticInputVarNode(size_t index);

  void eraseDynamicInputVarNode(size_t index);

  void replaceStaticInputVarNode(VarNodeId oldInputVarNodeId,
                                 VarNodeId newInputVarNodeId);

  void replaceDynamicInputVarNode(VarNodeId oldInputVarNodeId,
                                  VarNodeId newInputVarNodeId);

  [[nodiscard]] std::vector<std::pair<VarNodeId, VarNodeId>>
  splitOutputVarNodes();

  propagation::VarViewId makeSolverVar(VarNodeId varNodeId, propagation::SolverBase&, SolverMapping&) const;

  propagation::VarViewId makeSolverVar(VarNodeId varNodeId, Int initialValue, propagation::SolverBase&, SolverMapping&) const;

  void markOutputTo(VarNodeId varNodeId, bool registerHere);

  void markStaticInputTo(VarNodeId varNodeId, bool registerHere);

  void markDynamicInputTo(VarNodeId varNodeId, bool registerHere);

  virtual void registerOutputVars(propagation::SolverBase&, SolverMapping&) const = 0;

  virtual void registerNode(propagation::SolverBase&, SolverMapping&) const = 0;

  [[nodiscard]] virtual std::string dotLangIdentifier() const = 0;

  virtual std::ostream& dotLangEdges(std::ostream&) const;

  virtual std::ostream& dotLangEntry(std::ostream&) const;
};

}  // namespace atlantis::invariantgraph
