#pragma once

#include <map>
#include <utility>

#include "../structure.hpp"

namespace invariantgraph {

class Bool2IntNode : public VariableDefiningNode {
 private:
  VariableNode* _input;

 public:
  static std::unique_ptr<Bool2IntNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
          variableMap);

  Bool2IntNode(VariableNode* input, VariableNode* output)
      : VariableDefiningNode({output}, {input}), _input(input) {
    auto expectedBounds = std::make_pair<Int, Int>(0, 1);
    assert(output->bounds() == expectedBounds);
  }

  ~Bool2IntNode() override = default;

  void registerWithEngine(Engine& engine,
                          std::map<VariableNode*, VarId>& variableMap) override;

  [[nodiscard]] VariableNode* input() const noexcept { return _input; }
};

}  // namespace invariantgraph