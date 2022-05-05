#pragma once

#include "binaryOpNode.hpp"
#include "invariants/mod.hpp"

namespace invariantgraph {

class IntModNode : public BinaryOpNode {
 public:
  static inline std::string_view constraint_name() noexcept {
    return "int_mod";
  }

  IntModNode(VariableNode* a, VariableNode* b, VariableNode* output)
      : BinaryOpNode(a, b, output) {
    assert(definedVariables().size() == 1);
    assert(definedVariables().front() == output);
    assert(staticInputs().size() == 2);
    assert(staticInputs().front() == a);
    assert(staticInputs().back() == b);
    assert(dynamicInputs().empty());
  }

  ~IntModNode() override = default;

 protected:
  void createInvariant(Engine& engine, VarId a, VarId b,
                       VarId output) const override {
    engine.makeInvariant<::Mod>(a, b, output);
  }
};

}  // namespace invariantgraph