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
    assert(definedVariables().size() == 1);
    assert(definedVariables().front() == output);
    assert(staticInputs().size() == 2);
    assert(staticInputs().front() == a);
    assert(staticInputs().back() == b);
    assert(dynamicInputs().empty());
  }

  ~IntTimesNode() override = default;

 protected:
  void createInvariant(Engine& engine, VarId a, VarId b,
                       VarId output) const override {
    engine.makeInvariant<::Times>(a, b, output);
  }
};

}  // namespace invariantgraph