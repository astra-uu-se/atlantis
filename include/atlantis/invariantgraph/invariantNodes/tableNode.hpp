#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
class TableNode : public InvariantNode {
  std::vector<std::vector<Int>> _table;

  [[nodiscard]] VarNodeId numCols() const;
  [[nodiscard]] VarNodeId colVar(size_t index) const;

  void removeColumn(size_t colIndex);

  void removeColumns();
  void removeDuplicateColumns();
  void removeRows();

 public:
  explicit TableNode(InvariantGraph& graph, std::vector<VarNodeId>&& outputs,
                     VarNodeId input, std::vector<std::vector<Int>>&& table,
                     size_t inputColumnIndex);
  explicit TableNode(InvariantGraph& graph, std::vector<VarNodeId>&& outputs,
                     VarNodeId input,
                     const std::vector<std::vector<bool>>& table,
                     size_t inputColumnIndex);

  void init(InvariantNodeId) override;

  void postConstraint() override;

  void removeOutputVarNode(VarNodeId) override;

  void removeOutputAtIndex(size_t) override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  void updateState() override;

  [[nodiscard]] bool constrainsOutput(VarNodeId outputVarNodeId) const override;

  [[nodiscard]] std::pair<size_t, size_t> implicitRank() const override;

  [[nodiscard]] bool canBeMadeImplicit() const override;

  [[nodiscard]] bool makeImplicit() override;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
