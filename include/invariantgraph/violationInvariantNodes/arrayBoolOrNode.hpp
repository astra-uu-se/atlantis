#pragma once

#include <utility>

#include "fznparser/model.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "invariants/exists.hpp"
#include "views/notEqualConst.hpp"

namespace invariantgraph {

class ArrayBoolOrNode : public ViolationInvariantNode {
 private:
  VarId _intermediate{NULL_ID};

 public:
  ArrayBoolOrNode(std::vector<VarNodeId>&& as, VarNodeId output);

  ArrayBoolOrNode(std::vector<VarNodeId>&& as, bool shouldHold);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{{"array_bool_or", 2}};
  }

  static std::unique_ptr<ArrayBoolOrNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;
};

}  // namespace invariantgraph