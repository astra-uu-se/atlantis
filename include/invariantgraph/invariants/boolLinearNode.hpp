#pragma once

#include <algorithm>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/variableDefiningNode.hpp"
#include "invariants/boolLinear.hpp"
#include "views/intOffsetView.hpp"
#include "views/scalarView.hpp"

namespace invariantgraph {
class BoolLinearNode : public VariableDefiningNode {
 private:
  std::vector<Int> _coeffs;
  Int _definingCoefficient;
  Int _sum;
  VarId _intermediateVarId{NULL_ID};

 public:
  BoolLinearNode(std::vector<Int> coeffs, std::vector<VariableNode*> variables,
                 VariableNode* output, Int definingCoefficient, Int sum)
      : VariableDefiningNode({output}, std::move(variables)),
        _coeffs(std::move(coeffs)),
        _definingCoefficient(definingCoefficient),
        _sum(sum) {
    assert(std::all_of(
        staticInputs().begin(), staticInputs().end(),
        [&](auto* const staticInput) { return !staticInput->isIntVar(); }));
  }

  ~BoolLinearNode() override = default;

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"bool_lin_eq", 3}};
  }

  static std::unique_ptr<BoolLinearNode> fromModelConstraint(
      const fznparser::Model& model, const fznparser::Constraint& constraint,
      InvariantGraph& invariantGraph);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }
};
}  // namespace invariantgraph
