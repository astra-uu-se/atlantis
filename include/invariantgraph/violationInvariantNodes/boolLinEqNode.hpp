#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/constraints/equal.hpp"
#include "propagation/invariants/boolLinear.hpp"
#include "propagation/views/equalConst.hpp"
#include "propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

class BoolLinEqNode : public ViolationInvariantNode {
 private:
  std::vector<Int> _coeffs;
  Int _c;
  propagation::VarId _cVarId{propagation::NULL_ID};
  propagation::VarId _sumVarId{propagation::NULL_ID};

 public:
  BoolLinEqNode(std::vector<Int>&& coeffs, std::vector<VarNodeId>&& vars,
                Int c, VarNodeId r);

  BoolLinEqNode(std::vector<Int>&& coeffs, std::vector<VarNodeId>&& vars,
                Int c, bool shouldHold);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{{"bool_lin_eq", 3}};
  }

  static std::unique_ptr<BoolLinEqNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVars(InvariantGraph&,
                               propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase&) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }

  [[nodiscard]] Int c() const { return _c; }
};

}  // namespace atlantis::invariantgraph