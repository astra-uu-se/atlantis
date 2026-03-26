#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class TableInNode : public ViolationInvariantNode {
  std::vector<std::vector<Int>> _table;
  bool _isBoolTable;

  [[nodiscard]] VarNodeId numCols() const;

  bool removeFixedColumns();
  bool removeInvalidRows();

  void removeColumn(size_t colIndex);
  void removeDuplicateColumns();

  [[nodiscard]] bool propagate();

  [[nodiscard]] Int firstInputColIndex() const;

 public:
  explicit TableInNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
                       std::vector<std::vector<Int>>&& table, VarNodeId reified,
                       bool isBoolTable = false);

  explicit TableInNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
                       std::vector<std::vector<Int>>&& table,
                       bool shouldHold = true, bool isBoolTable = false);

  explicit TableInNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
                       std::vector<std::vector<bool>>&& table,
                       VarNodeId reified);

  explicit TableInNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
                       std::vector<std::vector<bool>>&& table,
                       bool shouldHold = true);

  void init(InvariantNodeId) override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  [[nodiscard]] bool canBeMadeImplicit() const override;

  [[nodiscard]] bool makeImplicit() override;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
