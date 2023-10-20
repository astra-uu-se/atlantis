#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/equal.hpp"
#include "constraints/notEqual.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "invariants/exists.hpp"
#include "invariants/globalCardinalityOpen.hpp"
#include "invariants/linear.hpp"
#include "views/equalConst.hpp"
#include "views/notEqualConst.hpp"

namespace invariantgraph {
class GlobalCardinalityNode : public ViolationInvariantNode {
 private:
  const std::vector<VarNodeId> _inputs;
  const std::vector<Int> _cover;
  const std::vector<VarNodeId> _counts;
  std::vector<VarId> _intermediate{};
  std::vector<VarId> _violations{};

 public:
  explicit GlobalCardinalityNode(std::vector<VarNodeId>&& x,
                                 std::vector<Int>&& cover,
                                 std::vector<VarNodeId>&& counts, VarNodeId r);

  explicit GlobalCardinalityNode(std::vector<VarNodeId>&& x,
                                 std::vector<Int>&& cover,
                                 std::vector<VarNodeId>&& counts,
                                 bool shouldHold);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{
        {"fzn_global_cardinality", 3}, {"fzn_global_cardinality_reif", 4}};
  }

  static std::unique_ptr<GlobalCardinalityNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;

  [[nodiscard]] inline const std::vector<VarNodeId>& inputs() const {
    return _inputs;
  }

  [[nodiscard]] inline const std::vector<VarNodeId>& counts() const {
    return _counts;
  }
};
}  // namespace invariantgraph