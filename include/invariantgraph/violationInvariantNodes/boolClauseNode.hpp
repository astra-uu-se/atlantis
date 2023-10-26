#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/invariants/boolLinear.hpp"
#include "propagation/views/equalConst.hpp"
#include "propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {
class BoolClauseNode : public ViolationInvariantNode {
 private:
  std::vector<VarNodeId> _as;
  std::vector<VarNodeId> _bs;
  propagation::VarId _sumVarId{propagation::NULL_ID};

 public:
  explicit BoolClauseNode(std::vector<VarNodeId>&& as,
                          std::vector<VarNodeId>&& bs, VarNodeId r);

  explicit BoolClauseNode(std::vector<VarNodeId>&& as,
                          std::vector<VarNodeId>&& bs, bool shouldHold);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{{"bool_clause", 2},
                                                       {"bool_clause_reif", 3}};
  }

  static std::unique_ptr<BoolClauseNode> fromModelConstraint(
      const fznparser::Constraint&, FznInvariantGraph&);

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] const std::vector<VarNodeId>& as() { return _as; }

  [[nodiscard]] const std::vector<VarNodeId>& bs() { return _bs; }
};
}  // namespace atlantis::invariantgraph