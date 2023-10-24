#pragma once

#include <fznparser/model.hpp>

#include "propagation/violationInvariants/notEqual.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/invariants/linear.hpp"
#include "propagation/views/equalConst.hpp"
#include "propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

class IntLinNeNode : public ViolationInvariantNode {
 private:
  std::vector<Int> _coeffs;
  Int _c;
  propagation::VarId _sumVarId{propagation::NULL_ID};

 public:
  IntLinNeNode(std::vector<Int>&& coeffs, std::vector<VarNodeId>&& vars,
               Int c, VarNodeId r);

  IntLinNeNode(std::vector<Int>&& coeffs, std::vector<VarNodeId>&& vars,
               Int c, bool shouldHold);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{{"int_lin_ne", 3},
                                                       {"int_lin_ne_reif", 4}};
  }

  static std::unique_ptr<IntLinNeNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVars(InvariantGraph&, propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }

  [[nodiscard]] Int c() const { return _c; }
};

}  // namespace invariantgraph