#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {

class ArrayBoolXorNode : public ViolationInvariantNode {
 public:
  ArrayBoolXorNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                   VarNodeId reified);

  ArrayBoolXorNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                   bool shouldHold = true);

  ArrayBoolXorNode(InvariantGraph& graph, std::vector<VarNodeId>&& inputs,
                   VarNodeId reified);

  ArrayBoolXorNode(InvariantGraph& graph, std::vector<VarNodeId>&& inputs,
                   bool shouldHold = true);

  void init(InvariantNodeId) override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  bool replace() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;
  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
