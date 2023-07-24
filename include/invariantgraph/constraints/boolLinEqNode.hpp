#pragma once

#include <fznparser/model.hpp>

#include "constraints/equal.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/softConstraintNode.hpp"
#include "invariants/boolLinear.hpp"
#include "views/equalConst.hpp"
#include "views/notEqualConst.hpp"

namespace invariantgraph {

class BoolLinEqNode : public SoftConstraintNode {
 private:
  std::vector<Int> _coeffs;
  Int _c;
  VarId _cVarId{NULL_ID};
  VarId _sumVarId{NULL_ID};

 public:
  BoolLinEqNode(std::vector<Int>&& coeffs,
                std::vector<VariableNode*>&& variables, Int c, VariableNode* r)
      : SoftConstraintNode(variables, r), _coeffs(std::move(coeffs)), _c(c) {}
  BoolLinEqNode(std::vector<Int>&& coeffs,
                std::vector<VariableNode*>&& variables, Int c, bool shouldHold)
      : SoftConstraintNode(variables, shouldHold),
        _coeffs(std::move(coeffs)),
        _c(c) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"bool_lin_eq", 3}};
  }

  static std::unique_ptr<BoolLinEqNode> fromModelConstraint(
      const fznparser::Model& model, const fznparser::Constraint& constraint,
      InvariantGraph& invariantGraph);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& node) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }

  [[nodiscard]] Int c() const { return _c; }
};

}  // namespace invariantgraph