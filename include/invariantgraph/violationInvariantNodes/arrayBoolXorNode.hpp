#pragma once

#include <utility>

#include "fznparser/model.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "invariants/boolLinear.hpp"
#include "views/equalConst.hpp"
#include "views/notEqualConst.hpp"

namespace invariantgraph {

class ArrayBoolXorNode : public ViolationInvariantNode {
 private:
  VarId _intermediate{NULL_ID};

 public:
  ArrayBoolXorNode(std::vector<VarNodeId>&& as, VarNodeId output);

  ArrayBoolXorNode(std::vector<VarNodeId>&& as, bool shouldHold);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{{"array_bool_xor", 1}};
  }

  static std::unique_ptr<ArrayBoolXorNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;
};

}  // namespace invariantgraph