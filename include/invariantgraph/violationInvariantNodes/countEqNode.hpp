#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "invariants/count.hpp"
#include "invariants/countConst.hpp"
#include "views/equalConst.hpp"
#include "views/notEqualConst.hpp"

static std::vector<invariantgraph::VarNodeId>&& append(
    std::vector<invariantgraph::VarNodeId>&& vars,
    invariantgraph::VarNodeId y) {
  if (y != nullptr) {
    vars.emplace_back(y);
  }
  return std::move(vars);
}

namespace invariantgraph {
class CountEqNode : public ViolationInvariantNode {
 private:
  const bool _yIsParameter;
  const Int _yParameter;
  const bool _cIsParameter;
  const Int _cParameter;
  VarId _intermediate{NULL_ID};

  explicit CountEqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int yParameter,
                       VarNodeId c, Int cParameter, VarNodeId r)
      : ViolationInvariantNode(
            id,
            c == nullptr ? std::vector<VarNodeId>{} : std::vector<VarNodeId>{c},
            append(std::move(x), y), r),
        _yIsParameter(y == nullptr),
        _yParameter(yParameter),
        _cIsParameter(c == nullptr),
        _cParameter(cParameter) {}

  explicit CountEqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int yParameter,
                       VarNodeId c, Int cParameter, bool shouldHold)
      : ViolationInvariantNode(
            id,
            c == nullptr ? std::vector<VarNodeId>{} : std::vector<VarNodeId>{c},
            append(std::move(x), y), shouldHold),
        _yIsParameter(y == nullptr),
        _yParameter(yParameter),
        _cIsParameter(c == nullptr),
        _cParameter(cParameter) {}

 public:
  explicit CountEqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int cParameter,
                       VarNodeId r)
      : CountEqNode(id, std::move(x), y, 0, nullptr, cParameter, r) {}

  explicit CountEqNode(std::vector<VarNodeId>&& x, Int yParameter,
                       Int cParameter, VarNodeId r)
      : CountEqNode(id, std::move(x), nullptr, yParameter, nullptr, cParameter,
                    r) {}

  explicit CountEqNode(std::vector<VarNodeId>&& x, VarNodeId y, VarNodeId c,
                       bool shouldHold)
      : CountEqNode(id, std::move(x), y, 0, c, 0, shouldHold) {}

  explicit CountEqNode(std::vector<VarNodeId>&& x, Int yParameter, VarNodeId c,
                       bool shouldHold)
      : CountEqNode(id, std::move(x), nullptr, yParameter, c, 0, shouldHold) {}

  explicit CountEqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int cParameter,
                       bool shouldHold)
      : CountEqNode(id, std::move(x), y, 0, nullptr, cParameter, shouldHold) {}

  explicit CountEqNode(std::vector<VarNodeId>&& x, Int yParameter,
                       Int cParameter, bool shouldHold)
      : CountEqNode(id, std::move(x), nullptr, yParameter, nullptr, cParameter,
                    shouldHold) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"fzn_count_eq", 3}, {"fzn_count_eq_reif", 4}};
  }

  static std::unique_ptr<CountEqNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;

  VarNodeId yVarNode() {
    if (_yIsParameter) {
      return nullptr;
    }
    return staticInputVarNodeIds().back();
  }

  VarNodeId cVarNode() {
    if (_cIsParameter) {
      return nullptr;
    }
    return outputVarNodeIds().back();
  }
};
}  // namespace invariantgraph