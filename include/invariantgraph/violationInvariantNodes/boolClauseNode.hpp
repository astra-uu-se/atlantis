#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "invariants/boolLinear.hpp"
#include "views/equalConst.hpp"
#include "views/notEqualConst.hpp"

namespace invariantgraph {
class BoolClauseNode : public ViolationInvariantNode {
 private:
  std::vector<VarNodeId> _as;
  std::vector<VarNodeId> _bs;
  VarId _sumVarId{NULL_ID};

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
      const fznparser::Constraint& constraint, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;

  [[nodiscard]] const std::vector<VarNodeId>& as() { return _as; }

  [[nodiscard]] const std::vector<VarNodeId>& bs() { return _bs; }
};
}  // namespace invariantgraph