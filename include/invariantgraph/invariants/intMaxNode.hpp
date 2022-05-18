#pragma once

#include <cmath>

#include "binaryOpNode.hpp"
#include "invariants/binaryMax.hpp"

namespace invariantgraph {

class IntMaxNode : public BinaryOpNode {
 public:
  static inline std::string_view constraint_name() noexcept {
    return "int_max";
  }

  IntMaxNode(VariableNode* a, VariableNode* b, VariableNode* output)
      : BinaryOpNode(a, b, output) {
#ifndef NDEBUG
    for (auto* const staticInput : staticInputs()) {
      assert(staticInput->isIntVar());
    }
#endif
  }

  ~IntMaxNode() override = default;

 protected:
  void createInvariant(Engine& engine, VarId a, VarId b,
                       VarId output) const override {
    engine.makeInvariant<BinaryMax>(a, b, output);
  }
};

}  // namespace invariantgraph