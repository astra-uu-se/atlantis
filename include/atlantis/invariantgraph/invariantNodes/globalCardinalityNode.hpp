#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
class GlobalCardinalityNode : public InvariantNode {
  std::vector<Int> _cover;
  std::vector<Int> _countOffsets;

 public:
  explicit GlobalCardinalityNode(InvariantGraph& graph,

                                 std::vector<VarNodeId>&& inputs,
                                 std::vector<Int>&& cover,
                                 std::vector<VarNodeId>&& counts);

  void init(InvariantNodeId) override;

  void registerOutputVars(propagation::SolverBase&, SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;
  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
