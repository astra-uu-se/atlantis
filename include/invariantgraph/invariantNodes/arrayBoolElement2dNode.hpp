#pragma once

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "invariantgraph/invariantNodes/arrayIntElement2dNode.hpp"
#include "propagation/invariants/element2dVar.hpp"

namespace atlantis::invariantgraph {

class ArrayBoolElement2dNode : public InvariantNode {
 public:
  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{
        {"array_bool_element2d_nonshifted_flat", 7}};
  }
};

}  // namespace atlantis::invariantgraph