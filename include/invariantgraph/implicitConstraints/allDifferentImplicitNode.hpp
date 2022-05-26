#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/implicitConstraintNode.hpp"

namespace invariantgraph {

class AllDifferentImplicitNode : public ImplicitConstraintNode {
 public:
  explicit AllDifferentImplicitNode(std::vector<VariableNode*> variables);

  ~AllDifferentImplicitNode() override = default;

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"fzn_all_different_int", 1}};
  }

  static std::unique_ptr<AllDifferentImplicitNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

 protected:
  search::neighbourhoods::Neighbourhood* createNeighbourhood(
      Engine& engine, std::vector<search::SearchVariable> variables) override;
};

}  // namespace invariantgraph