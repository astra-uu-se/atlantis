#pragma once

#include "fznparser/model.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "invariants/forAll.hpp"
#include "views/elementConst.hpp"
#include "views/notEqualConst.hpp"

namespace invariantgraph {

class ArrayBoolAndNode : public ViolationInvariantNode {
 private:
  VarId _intermediate{NULL_ID};

 public:
  ArrayBoolAndNode(std::vector<VarNodeId>&& as, VarNodeId output);

  ArrayBoolAndNode(std::vector<VarNodeId>&& as, bool shouldHold);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{{"array_bool_and", 2}};
  }

  static std::unique_ptr<ArrayBoolAndNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;
};

}  // namespace invariantgraph