#pragma once

#include <cmath>

#include "binaryOpNode.hpp"
#include "invariants/binaryMin.hpp"

namespace invariantgraph {

class IntMinNode : public BinaryOpNode {
 public:
  static inline std::string_view constraint_name() noexcept {
    return "int_min";
  }

  IntMinNode(VariableNode* a, VariableNode* b, VariableNode* output)
      : BinaryOpNode(a, b, output) {
#ifndef NDEBUG
    for (auto* const staticInput : staticInputs()) {
      assert(staticInput->isIntVar());
    }
#endif
  }

  ~IntMinNode() override = default;

 protected:
  void createInvariant(Engine& engine, VarId a, VarId b,
                       VarId output) const override {
    engine.makeInvariant<BinaryMin>(a, b, output);
  }
};

}  // namespace invariantgraph