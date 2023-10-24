#pragma once

#include <algorithm>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "propagation/invariants/linear.hpp"
#include "propagation/views/intOffsetView.hpp"
#include "propagation/views/scalarView.hpp"

namespace atlantis::invariantgraph {
class IntLinearNode : public InvariantNode {
 private:
  std::vector<Int> _coeffs;
  Int _definingCoefficient;
  Int _sum;
  propagation::VarId _intermediateVarId{propagation::NULL_ID};

 public:
  IntLinearNode(std::vector<Int>&& coeffs, std::vector<VarNodeId>&& vars,
                VarNodeId output, Int definingCoefficient, Int sum);

  ~IntLinearNode() override = default;

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{{"int_lin_eq", 3}};
  }

  static std::unique_ptr<IntLinearNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }
};
}  // namespace atlantis::invariantgraph
