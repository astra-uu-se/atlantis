#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {

class AllDifferentImplicitNode : public ImplicitConstraintNode {
 public:
  static std::unique_ptr<AllDifferentImplicitNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  explicit AllDifferentImplicitNode(std::vector<VariableNode*> variables);

  ~AllDifferentImplicitNode() override = default;

 protected:
  search::neighbourhoods::Neighbourhood* createNeighbourhood(
      Engine& engine, std::vector<search::SearchVariable> variables) override;
};

}  // namespace invariantgraph