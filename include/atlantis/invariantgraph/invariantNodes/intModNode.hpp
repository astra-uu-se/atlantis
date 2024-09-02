#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class IntModNode : public InvariantNode {
 public:
  IntModNode(IInvariantGraph& graph,

             VarNodeId numerator, VarNodeId denominator, VarNodeId remainder);

  void init(InvariantNodeId) override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] VarNodeId numerator() const;
  [[nodiscard]] VarNodeId denominator() const;
  [[nodiscard]] VarNodeId remainder() const;

  virtual std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
