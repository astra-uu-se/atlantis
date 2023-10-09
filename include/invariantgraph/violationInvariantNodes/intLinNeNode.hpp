#pragma once

#include <fznparser/model.hpp>

#include "constraints/notEqual.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "invariants/linear.hpp"
#include "views/equalConst.hpp"
#include "views/notEqualConst.hpp"

namespace invariantgraph {

class IntLinNeNode : public ViolationInvariantNode {
 private:
  std::vector<Int> _coeffs;
  Int _c;
  VarId _sumVarId{NULL_ID};

 public:
  IntLinNeNode(std::vector<Int>&& coeffs, std::vector<VarNodeId>&& variables,
               Int c, VarNodeId r);

  IntLinNeNode(std::vector<Int>&& coeffs, std::vector<VarNodeId>&& variables,
               Int c, bool shouldHold);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{{"int_lin_ne", 3},
                                                       {"int_lin_ne_reif", 4}};
  }

  static std::unique_ptr<IntLinNeNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }

  [[nodiscard]] Int c() const { return _c; }
};

}  // namespace invariantgraph