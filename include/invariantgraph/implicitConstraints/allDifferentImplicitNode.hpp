#pragma once

#include <fznparser/model.hpp>
#include <numeric>

#include "invariantgraph/implicitConstraintNode.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "search/neighbourhoods/allDifferentNonUniformNeighbourhood.hpp"
#include "search/neighbourhoods/allDifferentUniformNeighbourhood.hpp"

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
      const fznparser::Model& model, const fznparser::Constraint& constraint,
      InvariantGraph& invariantGraph);

  bool prune() override;

 protected:
  search::neighbourhoods::Neighbourhood* createNeighbourhood(
      Engine& engine, std::vector<search::SearchVariable> variables) override;
};

}  // namespace invariantgraph