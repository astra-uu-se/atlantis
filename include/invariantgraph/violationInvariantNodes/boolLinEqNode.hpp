#pragma once

#include <fznparser/model.hpp>

#include "constraints/equal.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "invariants/boolLinear.hpp"
#include "views/equalConst.hpp"
#include "views/notEqualConst.hpp"

namespace invariantgraph {

class BoolLinEqNode : public ViolationInvariantNode {
 private:
  std::vector<Int> _coeffs;
  Int _c;
  VarId _cVarId{NULL_ID};
  VarId _sumVarId{NULL_ID};

 public:
  BoolLinEqNode(std::vector<Int>&& coeffs, std::vector<VarNodeId>&& variables,
                Int c, VarNodeId r);

  BoolLinEqNode(std::vector<Int>&& coeffs, std::vector<VarNodeId>&& variables,
                Int c, bool shouldHold);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{{"bool_lin_eq", 3}};
  }

  static std::unique_ptr<BoolLinEqNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine&) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }

  [[nodiscard]] Int c() const { return _c; }
};

}  // namespace invariantgraph