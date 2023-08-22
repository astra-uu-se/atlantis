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

static std::vector<invariantgraph::VarNodeId> append(
    std::vector<invariantgraph::VarNodeId>&& vars, invariantgraph::VarNodeId y,
    invariantgraph::VarNodeId c) {
  if (y != nullptr) {
    vars.emplace_back(y);
  }
  if (c != nullptr) {
    vars.emplace_back(c);
  }
  return std::move(vars);
}

namespace invariantgraph {
class CountNeqNode : public ViolationInvariantNode {
 private:
  const bool _yIsParameter;
  const Int _yParameter;
  const bool _cIsParameter;
  const Int _cParameter;
  VarId _intermediate{NULL_ID};

  explicit CountNeqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int yParameter,
                        VarNodeId c, Int cParameter, VarNodeId r)
      : ViolationInvariantNode(append(std::move(x), y, c), r),
        _yIsParameter(y == nullptr),
        _yParameter(yParameter),
        _cIsParameter(c == nullptr),
        _cParameter(cParameter) {}

  explicit CountNeqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int yParameter,
                        VarNodeId c, Int cParameter, bool shouldHold)
      : ViolationInvariantNode(append(std::move(x), y, c), shouldHold),
        _yIsParameter(y == nullptr),
        _yParameter(yParameter),
        _cIsParameter(c == nullptr),
        _cParameter(cParameter) {}

 public:
  explicit CountNeqNode(std::vector<VarNodeId>&& x, VarNodeId y, VarNodeId c,
                        VarNodeId r)
      : CountNeqNode(std::move(x), y, 0, c, 0, r) {}

  explicit CountNeqNode(std::vector<VarNodeId>&& x, Int yParameter, VarNodeId c,
                        VarNodeId r)
      : CountNeqNode(std::move(x), nullptr, yParameter, c, 0, r) {}

  explicit CountNeqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int cParameter,
                        VarNodeId r)
      : CountNeqNode(std::move(x), y, 0, nullptr, cParameter, r) {}

  explicit CountNeqNode(std::vector<VarNodeId>&& x, Int yParameter,
                        Int cParameter, VarNodeId r)
      : CountNeqNode(std::move(x), nullptr, yParameter, nullptr, cParameter,
                     r) {}

  explicit CountNeqNode(std::vector<VarNodeId>&& x, VarNodeId y, VarNodeId c,
                        bool shouldHold)
      : CountNeqNode(std::move(x), y, 0, c, 0, shouldHold) {}

  explicit CountNeqNode(std::vector<VarNodeId>&& x, Int yParameter, VarNodeId c,
                        bool shouldHold)
      : CountNeqNode(std::move(x), nullptr, yParameter, c, 0, shouldHold) {}

  explicit CountNeqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int cParameter,
                        bool shouldHold)
      : CountNeqNode(std::move(x), y, 0, nullptr, cParameter, shouldHold) {}

  explicit CountNeqNode(std::vector<VarNodeId>&& x, Int yParameter,
                        Int cParameter, bool shouldHold)
      : CountNeqNode(std::move(x), nullptr, yParameter, nullptr, cParameter,
                     shouldHold) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"fzn_count_neq", 3}, {"fzn_count_neq_reif", 4}};
  }

  static std::unique_ptr<CountNeqNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;

  [[nodiscard]] VarNodeId yVarNode() const {
    return _yIsParameter ? nullptr
                         : staticInputVarNodeIds().at(
                               staticInputVarNodeIds().size() -
                               (1 + static_cast<size_t>(!_cIsParameter)));
  }

  [[nodiscard]] VarNodeId cVarNode() const {
    return _cIsParameter ? nullptr : staticInputVarNodeIds().back();
  }
};
}  // namespace invariantgraph