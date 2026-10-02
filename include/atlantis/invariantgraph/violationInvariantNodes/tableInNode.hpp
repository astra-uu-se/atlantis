#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class TableInNode : public ViolationInvariantNode {
  std::vector<std::vector<Int>> _table;
  bool _isBoolTable;

  [[nodiscard]] size_t numCols() const;

  void removeInvalidColumns();
  void removeInvalidRows();

  void removeColumn(size_t colIndex);
  void removeDuplicateColumns();

  [[nodiscard]] Int firstInputColIndex() const;

 public:
  explicit TableInNode(InvariantGraph& graph,
                       std::vector<std::shared_ptr<VarNode>>&& vars,
                       std::vector<std::vector<Int>>&& table, VarNode& reified,
                       bool isBoolTable = false);

  explicit TableInNode(InvariantGraph& graph,
                       std::vector<std::shared_ptr<VarNode>>&& vars,
                       std::vector<std::vector<Int>>&& table,
                       bool shouldHold = true, bool isBoolTable = false);

  explicit TableInNode(InvariantGraph& graph,
                       std::vector<std::shared_ptr<VarNode>>&& vars,
                       const std::vector<std::vector<bool>>& table,
                       VarNode& reified);

  explicit TableInNode(InvariantGraph& graph,
                       std::vector<std::shared_ptr<VarNode>>&& vars,
                       const std::vector<std::vector<bool>>& table,
                       bool shouldHold = true);

  void init() override;

  void postConstraint() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  [[nodiscard]] std::pair<size_t, size_t> implicitRank() const override;

  [[nodiscard]] bool canBeMadeImplicit() const override;

  [[nodiscard]] bool makeImplicit() override;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
