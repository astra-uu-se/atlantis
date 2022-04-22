#pragma once

#include <fznparser/model.hpp>
#include <numeric>

#include "invariantgraph/parseHelper.hpp"
#include "invariantgraph/structure.hpp"
#include "search/neighbourhoods/circuitNeighbourhood.hpp"

namespace invariantgraph {

class CircuitImplicitNode : public ImplicitConstraintNode {
 public:
  static std::unique_ptr<CircuitImplicitNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  explicit CircuitImplicitNode(std::vector<VariableNode*> variables);

  ~CircuitImplicitNode() override = default;

 protected:
  search::neighbourhoods::Neighbourhood* createNeighbourhood(
      Engine& engine, std::vector<search::SearchVariable> variables) override;
};

}  // namespace invariantgraph