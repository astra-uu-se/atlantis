#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
class TableNode : public InvariantNode {
  std::vector<std::vector<Int>> _table;

  [[nodiscard]] size_t numCols() const;
  [[nodiscard]] VarNode& colVar(size_t index) const;

  void removeColumn(size_t colIndex);

  void removeColumns();
  void removeDuplicateColumns();
  void removeRows();

 public:
  explicit TableNode(InvariantGraph& graph,
                     std::vector<std::shared_ptr<VarNode>>&& outputs,
                     VarNode& input, std::vector<std::vector<Int>>&& table,
                     size_t inputColumnIndex);
  explicit TableNode(InvariantGraph& graph,
                     std::vector<std::shared_ptr<VarNode>>&& outputs,
                     VarNode& input,
                     const std::vector<std::vector<bool>>& table,
                     size_t inputColumnIndex);

  void init() override;

  void postConstraint() override;

  void removeOutputVarNode(VarNode&) override;

  void removeOutputAtIndex(size_t) override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  void updateState() override;

  [[nodiscard]] bool constrainsOutput(
      const VarNode& outputVarNode) const override;

  [[nodiscard]] std::pair<size_t, size_t> implicitRank() const override;

  [[nodiscard]] bool canBeMadeImplicit() const override;

  [[nodiscard]] bool makeImplicit() override;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
