#pragma once

#include <algorithm>
#include <utility>

#include "constraints/lessEqual.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/softConstraintNode.hpp"
#include "invariants/boolLinear.hpp"
#include "views/greaterEqualConst.hpp"
#include "views/lessEqualConst.hpp"

namespace invariantgraph {

class BoolLinLeNode : public SoftConstraintNode {
 private:
  std::vector<Int> _coeffs;
  Int _bound;
  VarId _sumVarId{NULL_ID};

 public:
  BoolLinLeNode(std::vector<Int> coeffs, std::vector<VariableNode*> variables,
                Int bound, VariableNode* r)
      : SoftConstraintNode(variables, r),
        _coeffs(std::move(coeffs)),
        _bound(bound) {}
  BoolLinLeNode(std::vector<Int> coeffs, std::vector<VariableNode*> variables,
                Int bound, bool shouldHold)
      : SoftConstraintNode(variables, shouldHold),
        _coeffs(std::move(coeffs)),
        _bound(bound) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"bool_lin_le", 3}};
  }

  static std::unique_ptr<BoolLinLeNode> fromModelConstraint(
      const fznparser::Model& model, const fznparser::Constraint& constraint,
      InvariantGraph& invariantGraph);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }

  [[nodiscard]] Int bound() const { return _bound; }
};

}  // namespace invariantgraph