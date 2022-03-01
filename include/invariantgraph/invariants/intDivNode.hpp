#pragma once

#include "binaryOpNode.hpp"
#include "invariants/intDiv.hpp"

namespace invariantgraph {

class IntDivNode : public BinaryOpNode {
 public:
  static inline std::string_view constraint_name() noexcept {
    return "int_div";
  }

  IntDivNode(VariableNode* a, VariableNode* b, VariableNode* output)
      : BinaryOpNode(a, b, output) {}

  ~IntDivNode() override = default;

 protected:
  void createInvariant(Engine& engine, VarId a, VarId b,
                       VarId output) const override {
    engine.makeInvariant<::IntDiv>(a, b, output);
  }
};

}  // namespace invariantgraph