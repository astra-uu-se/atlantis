#pragma once

#include <cmath>

#include "binaryOpNode.hpp"
#include "invariants/plus.hpp"

namespace invariantgraph {

class IntPlusNode : public BinaryOpNode {
 public:
  static inline std::string_view constraint_name() noexcept {
    return "int_plus";
  }

  IntPlusNode(VariableNode* a, VariableNode* b, VariableNode* output)
      : BinaryOpNode(a, b, output) {
#ifndef NDEBUG
    for (auto* const staticInput : staticInputs()) {
      assert(staticInput->isIntVar());
    }
#endif
  }

  ~IntPlusNode() override = default;

 protected:
  void createInvariant(Engine& engine, VarId a, VarId b,
                       VarId output) const override {
    engine.makeInvariant<Plus>(a, b, output);
  }
};

}  // namespace invariantgraph