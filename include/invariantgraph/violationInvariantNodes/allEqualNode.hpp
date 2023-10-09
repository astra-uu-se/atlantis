#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/allDifferent.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "views/equalConst.hpp"
#include "views/notEqualConst.hpp"

namespace invariantgraph {
class InvariantGraph;  // Forward declaration

class AllEqualNode : public ViolationInvariantNode {
 private:
  VarId _allDifferentViolationVarId{NULL_ID};

 public:
  explicit AllEqualNode(std::vector<VarNodeId>&& variables, VarNodeId r);

  explicit AllEqualNode(std::vector<VarNodeId>&& variables,
                        bool shouldHold = true);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{
        {"fzn_all_equal_int", 1}, {"fzn_all_equal_int_reif", 2}};
  }

  static std::unique_ptr<AllEqualNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;
};
}  // namespace invariantgraph