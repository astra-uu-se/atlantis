#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class IntTimesNode : public InvariantNode {
 private:
  Int _scalar{1};
  propagation::VarViewId _intermediate{propagation::NULL_ID};

 public:
  IntTimesNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
               VarNodeId output);

  void init(InvariantNodeId) override;

  void updateState() override;

  bool canBeReplaced() const override;

  bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;
  virtual std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
