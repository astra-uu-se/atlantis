#pragma once


#include <utility>


#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/views/inDomain.hpp"
#include "propagation/views/notEqualConst.hpp"
#include "utils/variant.hpp"

namespace atlantis::invariantgraph {
class SetInNode : public ViolationInvariantNode {
 private:
  std::vector<Int> _values;
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  explicit SetInNode(VarNodeId input, std::vector<Int>&& values, VarNodeId r);

  explicit SetInNode(VarNodeId input, std::vector<Int>&& values,
                     bool shouldHold);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{{"set_in", 2},
                                                       {"set_in_reif", 3}};
  }

  

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] const std::vector<Int>& values() { return _values; }
};
}  // namespace atlantis::invariantgraph