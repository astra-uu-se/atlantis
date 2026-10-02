#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
class GlobalCardinalityNode : public InvariantNode {
  std::vector<Int> _cover;
  std::vector<Int> _countOffsets;

 public:
  explicit GlobalCardinalityNode(InvariantGraph& graph,
                                 std::vector<std::shared_ptr<VarNode>>&& inputs,
                                 std::vector<Int>&& cover,
                                 std::vector<std::shared_ptr<VarNode>>&& counts,
                                 std::vector<Int>&& countOffsets = {});

  void init() override;

  void postConstraint() override;

  void removeOutputVarNode(VarNode&) override;

  void removeOutputAtIndex(size_t) override;

  void updateState() override;

  [[nodiscard]] bool constrainsOutput(
      VarNode& outputVarNode) const override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
