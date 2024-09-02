#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class IntDivNode : public InvariantNode {
 public:
  IntDivNode(IInvariantGraph& graph,

             VarNodeId numerator, VarNodeId denominator, VarNodeId quotient);

  void init(InvariantNodeId) override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  [[nodiscard]] VarNodeId numerator() const noexcept;
  [[nodiscard]] VarNodeId denominator() const noexcept;
  [[nodiscard]] VarNodeId quotient() const noexcept;

  virtual std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
