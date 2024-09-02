#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
class GlobalCardinalityNode : public InvariantNode {
 private:
  std::vector<Int> _cover;
  std::vector<Int> _countOffsets;
  std::vector<propagation::VarViewId> _intermediate;

 public:
  explicit GlobalCardinalityNode(IInvariantGraph& graph,

                                 std::vector<VarNodeId>&& inputs,
                                 std::vector<Int>&& cover,
                                 std::vector<VarNodeId>&& counts);

  void init(InvariantNodeId) override;

  void registerOutputVars() override;

  void registerNode() override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;
  virtual std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
