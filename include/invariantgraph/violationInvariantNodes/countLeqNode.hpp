#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/lessEqual.hpp"
#include "constraints/lessThan.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "invariants/count.hpp"
#include "invariants/countConst.hpp"
#include "views/equalConst.hpp"
#include "views/greaterEqualConst.hpp"
#include "views/lessEqualConst.hpp"
#include "views/notEqualConst.hpp"

static std::vector<invariantgraph::VarNodeId>&& append(
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
class CountLeqNode : public ViolationInvariantNode {
 private:
  const bool _yIsParameter;
  const Int _yParameter;
  const bool _cIsParameter;
  const Int _cParameter;
  VarId _intermediate{NULL_ID};

  explicit CountLeqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int yParameter,
                        VarNodeId c, Int cParameter, VarNodeId r)
      : ViolationInvariantNode(append(std::move(x), y, c), r),
        _yIsParameter(y == nullptr),
        _yParameter(yParameter),
        _cIsParameter(c == nullptr),
        _cParameter(cParameter) {}

  explicit CountLeqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int yParameter,
                        VarNodeId c, Int cParameter, bool shouldHold)
      : ViolationInvariantNode(append(std::move(x), y, c), shouldHold),
        _yIsParameter(y == nullptr),
        _yParameter(yParameter),
        _cIsParameter(c == nullptr),
        _cParameter(cParameter) {}

 public:
  explicit CountLeqNode(std::vector<VarNodeId>&& x, VarNodeId y, VarNodeId c,
                        VarNodeId r)
      : CountLeqNode(id, std::move(x), y, 0, c, 0, r) {}

  explicit CountLeqNode(std::vector<VarNodeId>&& x, Int yParameter, VarNodeId c,
                        VarNodeId r)
      : CountLeqNode(id, std::move(x), nullptr, yParameter, c, 0, r) {}

  explicit CountLeqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int cParameter,
                        VarNodeId r)
      : CountLeqNode(id, std::move(x), y, 0, nullptr, cParameter, r) {}

  explicit CountLeqNode(std::vector<VarNodeId>&& x, Int yParameter,
                        Int cParameter, VarNodeId r)
      : CountLeqNode(id, std::move(x), nullptr, yParameter, nullptr, cParameter,
                     r) {}

  explicit CountLeqNode(std::vector<VarNodeId>&& x, VarNodeId y, VarNodeId c,
                        bool shouldHold)
      : CountLeqNode(id, std::move(x), y, 0, c, 0, shouldHold) {}

  explicit CountLeqNode(std::vector<VarNodeId>&& x, Int yParameter, VarNodeId c,
                        bool shouldHold)
      : CountLeqNode(id, std::move(x), nullptr, yParameter, c, 0, shouldHold) {}

  explicit CountLeqNode(std::vector<VarNodeId>&& x, VarNodeId y, Int cParameter,
                        bool shouldHold)
      : CountLeqNode(id, std::move(x), y, 0, nullptr, cParameter, shouldHold) {}

  explicit CountLeqNode(std::vector<VarNodeId>&& x, Int yParameter,
                        Int cParameter, bool shouldHold)
      : CountLeqNode(id, std::move(x), nullptr, yParameter, nullptr, cParameter,
                     shouldHold) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"fzn_count_leq", 3}, {"fzn_count_leq_reif", 4}};
  }

  static std::unique_ptr<CountLeqNode> fromModelConstraint(
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