#pragma once

#include <cmath>

#include "binaryOpNode.hpp"
#include "invariants/pow.hpp"

namespace invariantgraph {

class IntPowNode : public BinaryOpNode {
 public:
  static inline std::string_view constraint_name() noexcept {
    return "int_pow";
  }

  IntPowNode(VariableNode* a, VariableNode* b, VariableNode* output)
      : BinaryOpNode(a, b, output) {
    const auto& [aLb, aUb] = a->domain();
    const auto& [bLb, bUb] = b->domain();

    output->imposeDomain({std::pow(aLb, bLb), std::pow(aUb, bUb)});
  }

  ~IntPowNode() override = default;

 protected:
  void createInvariant(Engine& engine, VarId a, VarId b,
                       VarId output) const override {
    engine.makeInvariant<::Pow>(a, b, output);
  }
};

}  // namespace invariantgraph