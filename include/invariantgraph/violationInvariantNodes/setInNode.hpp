#pragma once
#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "utils/variant.hpp"
#include "views/inDomain.hpp"
#include "views/notEqualConst.hpp"

namespace invariantgraph {
class SetInNode : public ViolationInvariantNode {
 private:
  std::vector<Int> _values;
  VarId _intermediate{NULL_ID};

 public:
  explicit SetInNode(VarNodeId input, std::vector<Int>&& values, VarNodeId r);

  explicit SetInNode(VarNodeId input, std::vector<Int>&& values,
                     bool shouldHold);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{{"set_in", 2},
                                                       {"set_in_reif", 3}};
  }

  static std::unique_ptr<SetInNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;

  [[nodiscard]] const std::vector<Int>& values() { return _values; }
};
}  // namespace invariantgraph