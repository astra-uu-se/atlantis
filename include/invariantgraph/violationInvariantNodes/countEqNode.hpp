#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/invariants/count.hpp"
#include "propagation/invariants/countConst.hpp"
#include "propagation/views/equalConst.hpp"
#include "propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {
class CountEqNode : public ViolationInvariantNode {
 private:
  const bool _yIsParameter;
  const Int _yParameter;
  const bool _cIsParameter;
  const Int _cParameter;
  propagation::VarId _intermediate{propagation::NULL_ID};

  explicit CountEqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int yParameter,
                       VarNodeId c, Int cParameter, VarNodeId r);

  explicit CountEqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int yParameter,
                       VarNodeId c, Int cParameter, bool shouldHold);

 public:
  explicit CountEqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int cParameter,
                       VarNodeId r);

  explicit CountEqNode(std::vector<VarNodeId>&& x, Int yParameter,
                       Int cParameter, VarNodeId r);

  explicit CountEqNode(std::vector<VarNodeId>&& x, VarNodeId y, VarNodeId c,
                       bool shouldHold);

  explicit CountEqNode(std::vector<VarNodeId>&& x, Int yParameter, VarNodeId c,
                       bool shouldHold);

  explicit CountEqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int cParameter,
                       bool shouldHold);

  explicit CountEqNode(std::vector<VarNodeId>&& x, Int yParameter,
                       Int cParameter, bool shouldHold);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{
        {"fzn_count_eq", 3}, {"fzn_count_eq_reif", 4}};
  }

  static std::unique_ptr<CountEqNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, propagation::Engine& engine) override;

  void registerNode(InvariantGraph&, propagation::Engine& engine) override;

  VarNodeId yVarNode() {
    if (_yIsParameter) {
      return NULL_NODE_ID;
    }
    return staticInputVarNodeIds().back();
  }

  VarNodeId cVarNode() {
    if (_cIsParameter) {
      return NULL_NODE_ID;
    }
    return outputVarNodeIds().back();
  }
};
}  // namespace invariantgraph