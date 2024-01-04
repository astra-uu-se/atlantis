#pragma once


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

  bool prune(InvariantGraph&) override;

 protected:
  std::shared_ptr<search::neighbourhoods::Neighbourhood> createNeighbourhood(
      propagation::SolverBase&, std::vector<search::SearchVar>&&) override;
};

}  // namespace atlantis::invariantgraph