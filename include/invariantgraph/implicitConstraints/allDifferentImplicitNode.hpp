#pragma once

#include <fznparser/model.hpp>
#include <numeric>

#include "invariantgraph/implicitConstraintNode.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "search/neighbourhoods/allDifferentNonUniformNeighbourhood.hpp"
#include "search/neighbourhoods/allDifferentUniformNeighbourhood.hpp"

namespace atlantis::invariantgraph {

class AllDifferentImplicitNode : public ImplicitConstraintNode {
 public:
  explicit AllDifferentImplicitNode(std::vector<VarNodeId>&&);

  ~AllDifferentImplicitNode() override = default;

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{
        {"fzn_all_different_int", 1}};
  }

  static std::unique_ptr<AllDifferentImplicitNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  bool prune(InvariantGraph&) override;

 protected:
  std::shared_ptr<search::neighbourhoods::Neighbourhood> createNeighbourhood(
      propagation::Engine& engine,
      std::vector<search::SearchVariable>&& variables) override;
};

}  // namespace atlantis::invariantgraph