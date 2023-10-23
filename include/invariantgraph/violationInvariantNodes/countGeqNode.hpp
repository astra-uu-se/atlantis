#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "propagation/constraints/lessEqual.hpp"
#include "propagation/constraints/lessThan.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/invariants/count.hpp"
#include "propagation/invariants/countConst.hpp"
#include "propagation/views/equalConst.hpp"
#include "propagation/views/greaterEqualConst.hpp"
#include "propagation/views/lessEqualConst.hpp"
#include "propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {
class CountGeqNode : public ViolationInvariantNode {
 private:
  const bool _yIsParameter;
  const Int _yParameter;
  const bool _cIsParameter;
  const Int _cParameter;
  propagation::VarId _intermediate{propagation::NULL_ID};

  explicit CountGeqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int yParameter,
                        VarNodeId c, Int cParameter, VarNodeId r);

  explicit CountGeqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int yParameter,
                        VarNodeId c, Int cParameter, bool shouldHold);

 public:
  explicit CountGeqNode(std::vector<VarNodeId>&& x, VarNodeId y, VarNodeId c,
                        VarNodeId r);

  explicit CountGeqNode(std::vector<VarNodeId>&& x, Int yParameter, VarNodeId c,
                        VarNodeId r);

  explicit CountGeqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int cParameter,
                        VarNodeId r);

  explicit CountGeqNode(std::vector<VarNodeId>&& x, Int yParameter,
                        Int cParameter, VarNodeId r);

  explicit CountGeqNode(std::vector<VarNodeId>&& x, VarNodeId y, VarNodeId c,
                        bool shouldHold);

  explicit CountGeqNode(std::vector<VarNodeId>&& x, Int yParameter, VarNodeId c,
                        bool shouldHold);

  explicit CountGeqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int cParameter,
                        bool shouldHold);

  explicit CountGeqNode(std::vector<VarNodeId>&& x, Int yParameter,
                        Int cParameter, bool shouldHold);

  static std::vector<std::pair<std::string, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{
        {"fzn_count_geq", 3}, {"fzn_count_geq_reif", 4}};
  }

  static std::unique_ptr<CountGeqNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, propagation::Engine& engine) override;

  void registerNode(InvariantGraph&, propagation::Engine& engine) override;

  [[nodiscard]] VarNodeId yVarNode() const;

  [[nodiscard]] VarNodeId cVarNode() const;
};
}  // namespace invariantgraph