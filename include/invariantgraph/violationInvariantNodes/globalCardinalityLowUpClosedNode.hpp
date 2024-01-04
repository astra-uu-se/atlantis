#pragma once


#include <utility>


#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/invariants/exists.hpp"
#include "propagation/invariants/linear.hpp"
#include "propagation/views/notEqualConst.hpp"
#include "propagation/violationInvariants/equal.hpp"
#include "propagation/violationInvariants/globalCardinalityConst.hpp"
#include "propagation/violationInvariants/notEqual.hpp"

namespace atlantis::invariantgraph {
class GlobalCardinalityLowUpClosedNode : public ViolationInvariantNode {
 private:
  const std::vector<Int> _cover;
  const std::vector<Int> _low;
  const std::vector<Int> _up;
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  explicit GlobalCardinalityLowUpClosedNode(std::vector<VarNodeId>&& x,
                                            std::vector<Int>&& cover,
                                            std::vector<Int>&& low,
                                            std::vector<Int>&& up, VarNodeId r);

  explicit GlobalCardinalityLowUpClosedNode(std::vector<VarNodeId>&& x,
                                            std::vector<Int>&& cover,
                                            std::vector<Int>&& low,
                                            std::vector<Int>&& up,
                                            bool shouldHold);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{
        {"fzn_global_cardinality_low_up_closed", 4},
        {"fzn_global_cardinality_low_up_closed_reif", 5}};
  }

  

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;
};
}  // namespace atlantis::invariantgraph