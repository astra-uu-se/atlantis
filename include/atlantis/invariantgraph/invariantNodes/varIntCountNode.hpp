#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
class VarIntCountNode : public InvariantNode {
 public:
  VarIntCountNode(InvariantGraph& graph,

                  std::vector<VarNodeId>&& vars, VarNodeId needle,
                  VarNodeId count);

  void init(InvariantNodeId) override;

  void registerOutputVars(propagation::SolverBase&, SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  [[nodiscard]] std::vector<VarNodeId> haystack() const;

  [[nodiscard]] VarNodeId needle() const;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
