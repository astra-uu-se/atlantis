#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/boolAllEqual.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "views/notEqualConst.hpp"

namespace invariantgraph {
class BoolAllEqualNode : public ViolationInvariantNode {
 private:
  VarId _intermediate{NULL_ID};

 public:
  explicit BoolAllEqualNode(std::vector<VarNodeId>&& variables, VarNodeId r);

  explicit BoolAllEqualNode(std::vector<VarNodeId>&& variables,
                            bool shouldHold = true);

  explicit BoolAllEqualNode(InvariantGraph& invariantGraph,
                            std::vector<VarNodeId>&& variables,
                            bool shouldHold = true);

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"fzn_all_equal_bool", 1}, {"fzn_all_equal_bool_reif", 2}};
  }

  static std::unique_ptr<BoolAllEqualNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  bool prune(InvariantGraph&) override;

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;
};
}  // namespace invariantgraph