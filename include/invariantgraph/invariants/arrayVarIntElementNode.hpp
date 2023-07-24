#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/variableDefiningNode.hpp"
#include "invariants/elementVar.hpp"

namespace invariantgraph {

class ArrayVarIntElementNode : public VariableDefiningNode {
 private:
  const Int _offset;

 public:
  ArrayVarIntElementNode(VariableNode* b, std::vector<VariableNode*> as,
                         VariableNode* output, Int offset)
      : VariableDefiningNode({output}, {b}, as), _offset(offset) {
    assert(std::all_of(
        staticInputs().begin(), staticInputs().end(),
        [&](auto* const staticInput) { return staticInput->isIntVar(); }));
    assert(std::all_of(
        dynamicInputs().begin(), dynamicInputs().end(),
        [&](auto* const dynamicInput) { return dynamicInput->isIntVar(); }));
  }

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"array_var_int_element", 3}, {"array_var_int_element_nonshifted", 3}};
  }

  static std::unique_ptr<ArrayVarIntElementNode> fromModelConstraint(
      const fznparser::Model& model, const fznparser::Constraint& constraint,
      InvariantGraph& invariantGraph);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] VariableNode* b() const noexcept {
    return staticInputs().front();
  }
};

}  // namespace invariantgraph