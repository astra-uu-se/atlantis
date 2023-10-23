#pragma once

#include <algorithm>
#include <utility>

#include "propagation/constraints/lessEqual.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/invariants/boolLinear.hpp"
#include "propagation/views/greaterEqualConst.hpp"
#include "propagation/views/lessEqualConst.hpp"

namespace atlantis::invariantgraph {

class BoolLinLeNode : public ViolationInvariantNode {
 private:
  std::vector<Int> _coeffs;
  Int _bound;
  propagation::VarId _sumVarId{propagation::NULL_ID};

 public:
  BoolLinLeNode(std::vector<Int> coeffs, std::vector<VarNodeId>&& variables,
                Int bound, VarNodeId r);

  BoolLinLeNode(std::vector<Int> coeffs, std::vector<VarNodeId>&& variables,
                Int bound, bool shouldHold);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{{"bool_lin_le", 3}};
  }

  static std::unique_ptr<BoolLinLeNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, propagation::Engine& engine) override;

  void registerNode(InvariantGraph&, propagation::Engine& engine) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }

  [[nodiscard]] Int bound() const { return _bound; }
};

}  // namespace invariantgraph