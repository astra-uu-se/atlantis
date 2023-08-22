#pragma once

#include <algorithm>
#include <fznparser/model.hpp>
#include <utility>

#include "constraints/lessEqual.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "invariants/linear.hpp"
#include "views/greaterEqualConst.hpp"
#include "views/lessEqualConst.hpp"

namespace invariantgraph {

class IntLinLeNode : public ViolationInvariantNode {
 private:
  std::vector<Int> _coeffs;
  Int _bound;
  VarId _sumVarId{NULL_ID};

 public:
  IntLinLeNode(std::vector<Int>&& coeffs, std::vector<VarNodeId>&& variables,
               Int bound, VarNodeId r)
      : ViolationInvariantNode(std::move(variables), r),
        _coeffs(std::move(coeffs)),
        _bound(bound) {}

  IntLinLeNode(std::vector<Int>&& coeffs, std::vector<VarNodeId>&& variables,
               Int bound, bool shouldHold)
      : ViolationInvariantNode(std::move(variables), shouldHold),
        _coeffs(std::move(coeffs)),
        _bound(bound) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"int_lin_le", 3}, {"int_lin_le_reif", 4}};
  }

  static std::unique_ptr<IntLinLeNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }

  [[nodiscard]] Int bound() const { return _bound; }
};

}  // namespace invariantgraph