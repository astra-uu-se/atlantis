#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
class TableNode : public InvariantNode {
  std::vector<std::vector<Int>> _table;
  size_t _inputColumnIndex;

  [[nodiscard]] VarNodeId numCols() const;
  [[nodiscard]] VarNodeId colVar(size_t index) const;

 public:
  explicit TableNode(InvariantGraph& graph,
                 std::vector<VarNodeId>&& outputs,
                 VarNodeId input,
                 std::vector<std::vector<Int>>&& table,
                 size_t inputColumnIndex);

  void init(InvariantNodeId) override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
