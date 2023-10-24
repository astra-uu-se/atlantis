#pragma once

#include <fznparser/model.hpp>
#include <numeric>

#include "invariantgraph/implicitConstraintNode.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "search/neighbourhoods/circuitNeighbourhood.hpp"

namespace atlantis::invariantgraph {

class CircuitImplicitNode : public ImplicitConstraintNode {
 public:
  explicit CircuitImplicitNode(std::vector<VarNodeId>&&);

  ~CircuitImplicitNode() override = default;

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{
        {"circuit_no_offset", 1}};
  }

  static std::unique_ptr<CircuitImplicitNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

 protected:
  std::shared_ptr<search::neighbourhoods::Neighbourhood> createNeighbourhood(
      propagation::SolverBase& solver,
      std::vector<search::SearchVar>&&) override;
};

}  // namespace atlantis::invariantgraph