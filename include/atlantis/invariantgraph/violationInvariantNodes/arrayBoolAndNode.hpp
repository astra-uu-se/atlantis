#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::invariantgraph {

class ArrayBoolAndNode : public ViolationInvariantNode {
 private:
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  ArrayBoolAndNode(VarNodeId a, VarNodeId b, VarNodeId output);

  ArrayBoolAndNode(VarNodeId a, VarNodeId b, bool shouldHold = true);

  ArrayBoolAndNode(std::vector<VarNodeId>&& as, VarNodeId output);

  ArrayBoolAndNode(std::vector<VarNodeId>&& as, bool shouldHold = true);

  void init(InvariantGraph&, const InvariantNodeId&) override;

  void updateState(InvariantGraph&) override;

  bool canBeReplaced(const InvariantGraph&) const override;

  bool replace(InvariantGraph&) override;

  void registerOutputVars(InvariantGraph&, propagation::SolverBase&) override;

  void registerNode(InvariantGraph&, propagation::SolverBase&) override;

  virtual std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
