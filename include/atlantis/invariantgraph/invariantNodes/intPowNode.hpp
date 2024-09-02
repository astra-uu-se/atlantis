#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class IntPowNode : public InvariantNode {
 public:
  IntPowNode(IInvariantGraph& graph, VarNodeId base, VarNodeId exponent,
             VarNodeId power);

  void init(InvariantNodeId) override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] VarNodeId base() const;
  [[nodiscard]] VarNodeId exponent() const;
  [[nodiscard]] VarNodeId power() const;

  virtual std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
