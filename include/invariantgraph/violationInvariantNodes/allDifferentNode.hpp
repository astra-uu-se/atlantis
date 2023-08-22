#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/allDifferent.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "views/notEqualConst.hpp"

namespace invariantgraph {
class AllDifferentNode : public ViolationInvariantNode {
 private:
  VarId _intermediate{NULL_ID};

 public:
  explicit AllDifferentNode(std::vector<VarNodeId>&& variables, VarNodeId r);

  explicit AllDifferentNode(std::vector<VarNodeId>&& variables,
                            bool shouldHold);

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"fzn_all_different_int", 1}, {"fzn_all_different_int_reif", 2}};
  }

  static std::unique_ptr<AllDifferentNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  bool prune(InvariantGraph&) override;

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;
};
}  // namespace invariantgraph