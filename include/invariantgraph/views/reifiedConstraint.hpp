#pragma once

#include <utility>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {

class ReifiedConstraint : public ViewNode {
 private:
  std::unique_ptr<SoftConstraintNode> _constraint;
  std::shared_ptr<fznparser::SearchVariable> _r;

 public:
  ReifiedConstraint(std::unique_ptr<SoftConstraintNode> constraint, std::shared_ptr<fznparser::SearchVariable> r)
      : ViewNode(nullptr, r), _constraint(std::move(constraint)), _r(std::move(r)) {}

  void registerWithEngine(Engine& engine,
                          std::map<VariableNode*, VarId>& map) override;

 protected:
  std::shared_ptr<View> createView(Engine &engine, VarId variable) const override;
};

}  // namespace invariantgraph
