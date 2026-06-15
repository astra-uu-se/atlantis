#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {

/**
 * CountRelNode(vars, needle, amount, relationType[, reified])
 *  with \var relationType \in {=, !=, <, <=, =>, >},
 *  Variable \var amount \var relationType the number of occurcences of \var
 *needle in \var vars.
 **/
class CountRelNode : public ViolationInvariantNode {
  std::optional<Int> _fixedNeedle;
  std::optional<Int> _fixedBound;
  Int _boundOffset{0};
  RelationType _relType;

  [[nodiscard]] VarNodeId needle() const;
  [[nodiscard]] size_t needleIndex() const;
  [[nodiscard]] VarNodeId bound() const;
  [[nodiscard]] size_t boundIndex() const;
  [[nodiscard]] size_t numInputVars() const;

 public:
  /**
   * @param graph the invariant graph this invariant is created for
   * @param amount the bound for the number of occurrences of \a needle in \a
   * vars
   * @param relationType the relation between \a amount and the number of
   * occurrences of \a needle in \a vars
   * @param vars the variables that are counted over
   * @param needle the variable that is to be counted
   * @param shouldHold true if the invariant should be satisfied, and false
   * otherwise
   */
  CountRelNode(InvariantGraph& graph, Int amount, RelationType relationType,
               std::vector<VarNodeId>&& vars, Int needle,
               bool shouldHold = true);

  CountRelNode(InvariantGraph& graph, Int amount, RelationType relationType,
               std::vector<VarNodeId>&& vars, Int needle, VarNodeId reified);

  CountRelNode(InvariantGraph& graph, VarNodeId amount,
               RelationType relationType, std::vector<VarNodeId>&& vars,
               Int needle, bool shouldHold = true);

  CountRelNode(InvariantGraph& graph, VarNodeId amount,
               RelationType relationType, std::vector<VarNodeId>&& vars,
               Int needle, VarNodeId reified);

  CountRelNode(InvariantGraph& graph, Int amount, RelationType relationType,
               std::vector<VarNodeId>&& vars, VarNodeId needle,
               bool shouldHold = true);

  CountRelNode(InvariantGraph& graph, Int amount, RelationType relationType,
               std::vector<VarNodeId>&& vars, VarNodeId needle,
               VarNodeId reified);

  CountRelNode(InvariantGraph& graph, VarNodeId amount,
               RelationType relationType, std::vector<VarNodeId>&& vars,
               VarNodeId needle, bool shouldHold = true);

  CountRelNode(InvariantGraph& graph, VarNodeId amount,
               RelationType relationType, std::vector<VarNodeId>&& vars,
               VarNodeId needle, VarNodeId reified);

  void init(InvariantNodeId) override;

  void postConstraint() override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  bool replace() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
