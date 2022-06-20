#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/variableDefiningNode.hpp"
#include "views/elementConst.hpp"

namespace invariantgraph {

class ArrayIntElementNode : public VariableDefiningNode {
 private:
  std::vector<Int> _as;
  const Int _offset;

 public:
  ArrayIntElementNode(std::vector<Int> as, VariableNode* b,
                      VariableNode* output, Int offset)
      : VariableDefiningNode({output}, {b}),
        _as(std::move(as)),
        _offset(offset) {
#ifndef NDEBUG
    for (auto* const staticInput : staticInputs()) {
      assert(staticInput->isIntVar());
    }
#endif
  }

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"array_int_element", 3}, {"array_int_element_offset", 4}};
  }

  static std::unique_ptr<ArrayIntElementNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] const std::vector<Int>& as() const noexcept { return _as; }
  [[nodiscard]] VariableNode* b() const noexcept {
    return staticInputs().back();
  }
};

}  // namespace invariantgraph