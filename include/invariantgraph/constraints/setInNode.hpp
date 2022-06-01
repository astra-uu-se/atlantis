#pragma once
#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/softConstraintNode.hpp"
#include "utils/variant.hpp"
#include "views/inDomain.hpp"
#include "views/notEqualConst.hpp"

namespace invariantgraph {
class SetInNode : public SoftConstraintNode {
 private:
  std::vector<Int> _values;
  VarId _intermediate{NULL_ID};

 public:
  explicit SetInNode(VariableNode* input, std::vector<Int> values,
                     VariableNode* r)
      : SoftConstraintNode({input}, r), _values(std::move(values)) {}
  explicit SetInNode(VariableNode* input, std::vector<Int> values,
                     bool shouldHold)
      : SoftConstraintNode({input}, shouldHold), _values(std::move(values)) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"set_in", 2},
                                                            {"set_in_reif", 3}};
  }

  static std::unique_ptr<SetInNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] const std::vector<Int>& values() { return _values; }
};
}  // namespace invariantgraph