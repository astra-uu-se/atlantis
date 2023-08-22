#pragma once

#include <algorithm>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "invariants/linear.hpp"
#include "views/intOffsetView.hpp"
#include "views/scalarView.hpp"

namespace invariantgraph {
class IntLinearNode : public InvariantNode {
 private:
  std::vector<Int> _coeffs;
  Int _definingCoefficient;
  Int _sum;
  VarId _intermediateVarId{NULL_ID};

 public:
  IntLinearNode(std::vector<Int>&& coeffs, std::vector<VarNodeId>&& variables,
                VarNodeId output, Int definingCoefficient, Int sum)
      : InvariantNode({output}, std::move(variables)),
        _coeffs(std::move(coeffs)),
        _definingCoefficient(definingCoefficient),
        _sum(sum) {}

  ~IntLinearNode() override = default;

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"int_lin_eq", 3}};
  }

  static std::unique_ptr<IntLinearNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }
};
}  // namespace invariantgraph
