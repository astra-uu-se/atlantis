#pragma once

#include "binaryOpNode.hpp"
#include "invariants/times.hpp"

namespace invariantgraph {

class IntTimesNode : public BinaryOpNode {
 public:
  static inline std::string_view constraint_name() noexcept {
    return "int_times";
  }

  IntTimesNode(VariableNode* a, VariableNode* b, VariableNode* output)
      : BinaryOpNode(a, b, output) {
#ifndef NDEBUG
    for (auto* const staticInput : staticInputs()) {
      assert(staticInput->isIntVar());
    }
#endif
  }

  ~IntTimesNode() override = default;

 protected:
  void createInvariant(Engine& engine, VarId a, VarId b,
                       VarId output) const override {
    engine.makeInvariant<::Times>(a, b, output);
  }
};

}  // namespace invariantgraph