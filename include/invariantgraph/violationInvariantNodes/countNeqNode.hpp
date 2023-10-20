#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/equal.hpp"
#include "constraints/notEqual.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "invariants/count.hpp"
#include "invariants/countConst.hpp"
#include "views/equalConst.hpp"
#include "views/notEqualConst.hpp"

namespace invariantgraph {
class CountNeqNode : public ViolationInvariantNode {
 private:
  const bool _yIsParameter;
  const Int _yParameter;
  const bool _cIsParameter;
  const Int _cParameter;
  VarId _intermediate{NULL_ID};

  explicit CountNeqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int yParameter,
                        VarNodeId c, Int cParameter, VarNodeId r);

  explicit CountNeqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int yParameter,
                        VarNodeId c, Int cParameter, bool shouldHold);

 public:
  explicit CountNeqNode(std::vector<VarNodeId>&& x, VarNodeId y, VarNodeId c,
                        VarNodeId r);

  explicit CountNeqNode(std::vector<VarNodeId>&& x, Int yParameter, VarNodeId c,
                        VarNodeId r);

  explicit CountNeqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int cParameter,
                        VarNodeId r);

  explicit CountNeqNode(std::vector<VarNodeId>&& x, Int yParameter,
                        Int cParameter, VarNodeId r);

  explicit CountNeqNode(std::vector<VarNodeId>&& x, VarNodeId y, VarNodeId c,
                        bool shouldHold);

  explicit CountNeqNode(std::vector<VarNodeId>&& x, Int yParameter, VarNodeId c,
                        bool shouldHold);

  explicit CountNeqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int cParameter,
                        bool shouldHold);

  explicit CountNeqNode(std::vector<VarNodeId>&& x, Int yParameter,
                        Int cParameter, bool shouldHold);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{
        {"fzn_count_neq", 3}, {"fzn_count_neq_reif", 4}};
  }

  static std::unique_ptr<CountNeqNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;

  [[nodiscard]] VarNodeId yVarNode() const;

  [[nodiscard]] VarNodeId cVarNode() const;
};
}  // namespace invariantgraph