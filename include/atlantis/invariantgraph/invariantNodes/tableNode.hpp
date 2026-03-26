#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
class TableNode : public InvariantNode {
  std::vector<std::vector<Int>> _table;
  bool _isBoolTable;

  [[nodiscard]] VarNodeId numCols() const;
  [[nodiscard]] VarNodeId colVar(size_t index) const;

  bool removeRows();

  void removeColumn(size_t colIndex);
  void removeDuplicateColumns();

  [[nodiscard]] bool propagate();

 public:
  explicit TableNode(InvariantGraph& graph, std::vector<VarNodeId>&& outputs,
                     VarNodeId input, std::vector<std::vector<Int>>&& table,
                     size_t inputColumnIndex, bool isBoolTable = false);
  explicit TableNode(InvariantGraph& graph, std::vector<VarNodeId>&& outputs,
                     VarNodeId input, std::vector<std::vector<bool>>&& table,
                     size_t inputColumnIndex);

  void init(InvariantNodeId) override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  void updateState() override;

  [[nodiscard]] bool canBeMadeImplicit() const override;

  [[nodiscard]] bool makeImplicit() override;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
