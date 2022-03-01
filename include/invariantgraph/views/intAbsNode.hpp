#pragma once

#include <map>
#include <utility>

#include "../structure.hpp"
#include "views/intAbsView.hpp"

namespace invariantgraph {

class IntAbsNode : public ViewNode {
 public:
  static std::unique_ptr<IntAbsNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
          variableMap);

  explicit IntAbsNode(VariableNode* input,
                      std::shared_ptr<fznparser::SearchVariable> output)
      : ViewNode(input, std::move(output)) {}
  ~IntAbsNode() override = default;

 protected:
  std::shared_ptr<View> createView(Engine& engine,
                                   VarId variable) const override {
    return engine.makeIntView<::IntAbsView>(variable);
  }
};

}  // namespace invariantgraph