#pragma once

#include <functional>
#include <memory>
#include <set>
#include <variant>

#include "fznparser/model.hpp"

namespace invariantgraph {
class VariableNode;
class InvariantNode {
 private:
  std::shared_ptr<fznparser::Constraint> modelConstraint;

 public:
  virtual ~InvariantNode() = default;

  virtual void registerWithEngine(
      Engine& engine,
      std::function<VarId(const std::shared_ptr<VariableNode>&)> variableMapper)
      const = 0;
};

class SoftConstraintNode {
 private:
  std::shared_ptr<fznparser::Constraint> modelConstraint;

 public:
  virtual ~SoftConstraintNode() = default;

  virtual VarId registerWithEngine(
      Engine& engine,
      std::function<VarId(const std::shared_ptr<VariableNode>&)> variableMapper)
      const = 0;

  [[nodiscard]] std::shared_ptr<VariableNode> violationNode() const;
};

class VariableNode {
 public:
  [[nodiscard]] std::shared_ptr<fznparser::SearchVariable> variable() const;
  [[nodiscard]] const std::set<std::shared_ptr<SoftConstraintNode>>&
  softConstraints() const;

  [[nodiscard]] std::optional<std::shared_ptr<InvariantNode>>
  definingInvariant() const;
  [[nodiscard]] bool isFunctionallyDefined() const {
    return definingInvariant().has_value();
  }
};

}  // namespace invariantgraph