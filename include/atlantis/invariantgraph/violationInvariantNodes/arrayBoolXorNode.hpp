#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::invariantgraph {

class ArrayBoolXorNode : public ViolationInvariantNode {
 private:
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  ArrayBoolXorNode(VarNodeId aVarNodeId, VarNodeId bNodeId,
                   VarNodeId reifiedVarNodeId);

  ArrayBoolXorNode(VarNodeId aVarNodeId, VarNodeId bNodeId,
                   bool shouldHold = true);

  ArrayBoolXorNode(std::vector<VarNodeId>&& inputVarNodeIds,
                   VarNodeId reifiedVarNodeId);

  ArrayBoolXorNode(std::vector<VarNodeId>&& inputVarNodeIds,
                   bool shouldHold = true);

  void init(InvariantGraph&, const InvariantNodeId&) override;

  void updateState(InvariantGraph&) override;

  bool canBeReplaced(const InvariantGraph&) const override;

  bool replace(InvariantGraph&) override;

  void registerOutputVars(InvariantGraph&, propagation::SolverBase&) override;

  void registerNode(InvariantGraph&, propagation::SolverBase&) override;

  virtual std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
