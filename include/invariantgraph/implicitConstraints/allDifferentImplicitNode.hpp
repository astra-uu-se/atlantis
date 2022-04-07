#pragma once

#include "invariantgraph/structure.hpp"

namespace invariantgraph {

class AllDifferentImplicitNode : public ImplicitConstraintNode {
 public:
  static std::unique_ptr<AllDifferentImplicitNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint> &constraint,
      const std::function<VariableNode *(std::shared_ptr<fznparser::Variable>)>
          &variableMap);

  explicit AllDifferentImplicitNode(std::vector<VariableNode *> variables);

  ~AllDifferentImplicitNode() override = default;

 protected:
  search::neighbourhoods::Neighbourhood *createNeighbourhood(
      Engine &engine, std::vector<search::SearchVariable> variables) override;
};

}  // namespace invariantgraph