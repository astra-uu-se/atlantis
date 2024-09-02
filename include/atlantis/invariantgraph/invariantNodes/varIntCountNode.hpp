#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
class VarIntCountNode : public InvariantNode {
 public:
  VarIntCountNode(IInvariantGraph& graph,

                  std::vector<VarNodeId>&& vars, VarNodeId needle,
                  VarNodeId count);

  void init(InvariantNodeId) override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  [[nodiscard]] std::vector<VarNodeId> haystack() const;

  [[nodiscard]] VarNodeId needle() const;

  virtual std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
