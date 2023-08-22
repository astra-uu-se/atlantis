#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "invariants/boolLinear.hpp"
#include "views/equalConst.hpp"
#include "views/notEqualConst.hpp"

static std::vector<invariantgraph::VarNodeId> merge(
    const std::vector<invariantgraph::VarNodeId>& as,
    const std::vector<invariantgraph::VarNodeId>& bs) {
  std::vector<invariantgraph::VarNodeId> output(as.size() + bs.size());
  for (size_t i = 0; i < as.size(); ++i) {
    output[i] = as[i];
  }
  for (size_t i = 0; i < bs.size(); ++i) {
    output[as.size() + i] = bs[i];
  }
  return output;
}

namespace invariantgraph {
class BoolClauseNode : public ViolationInvariantNode {
 private:
  std::vector<VarNodeId> _as;
  std::vector<VarNodeId> _bs;
  VarId _sumVarId{NULL_ID};

 public:
  explicit BoolClauseNode(std::vector<VarNodeId>&& as,
                          std::vector<VarNodeId>&& bs, VarNodeId r)
      : ViolationInvariantNode(std::move(merge(as, bs)), r),
        _as(std::move(as)),
        _bs(std::move(bs)) {}
  explicit BoolClauseNode(std::vector<VarNodeId>&& as,
                          std::vector<VarNodeId>&& bs, bool shouldHold)
      : ViolationInvariantNode(std::move(merge(as, bs)), shouldHold),
        _as(std::move(as)),
        _bs(std::move(bs)) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"bool_clause", 2}, {"bool_clause_reif", 3}};
  }

  static std::unique_ptr<BoolClauseNode> fromModelConstraint(
      const fznparser::Constraint& constraint, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;

  [[nodiscard]] const std::vector<VarNodeId>& as() { return _as; }

  [[nodiscard]] const std::vector<VarNodeId>& bs() { return _bs; }
};
}  // namespace invariantgraph