#pragma once

#include <functional>
#include <memory>
#include <set>
#include <utility>
#include <variant>

#include "core/engine.hpp"
#include "fznparser/model.hpp"

namespace invariantgraph {
class VariableNode;
class InvariantNode {
 private:
  std::shared_ptr<VariableNode> _output;

 public:
  explicit InvariantNode(std::shared_ptr<VariableNode> output)
      : _output(std::move(output)) {}
  virtual ~InvariantNode() = default;

  virtual void registerWithEngine(
      Engine& engine,
      std::function<VarId(const std::shared_ptr<VariableNode>&)> variableMapper)
      const = 0;

  [[nodiscard]] std::shared_ptr<VariableNode> output() const { return _output; }
};

class SoftConstraintNode {
 public:
  virtual ~SoftConstraintNode() = default;

  virtual VarId registerWithEngine(
      Engine& engine,
      std::function<VarId(const std::shared_ptr<VariableNode>&)> variableMapper)
      const = 0;
};

class ImplicitConstraintNode {
 private:
  std::vector<std::shared_ptr<VariableNode>> _definingVariables;

 public:
  virtual ~ImplicitConstraintNode() = default;

  [[nodiscard]] const std::vector<std::shared_ptr<VariableNode>>&
  definingVariables() const {
    return _definingVariables;
  }
};

class VariableNode {
 private:
  std::shared_ptr<fznparser::SearchVariable> _variable;
  std::set<std::shared_ptr<SoftConstraintNode>> _softConstraints;
  std::set<std::shared_ptr<InvariantNode>> _invariants;
  std::optional<std::weak_ptr<InvariantNode>> _definingInvariant;
  Int _offset = 0;

 public:
  explicit VariableNode(std::shared_ptr<fznparser::SearchVariable> variable)
      : _variable(std::move(variable)) {}

  [[nodiscard]] std::shared_ptr<fznparser::SearchVariable> variable() const {
    return _variable;
  }

  void addSoftConstraint(std::shared_ptr<SoftConstraintNode> node) {
    _softConstraints.emplace(node);
  }

  [[nodiscard]] const std::set<std::shared_ptr<SoftConstraintNode>>&
  softConstraints() const {
    return _softConstraints;
  }

  void definedByInvariant(std::weak_ptr<InvariantNode> node) {
    _definingInvariant = node;
  }

  [[nodiscard]] std::optional<std::weak_ptr<InvariantNode>> definingInvariant()
      const {
    return _definingInvariant;
  }

  [[nodiscard]] bool isFunctionallyDefined() const {
    return definingInvariant().has_value();
  }

  [[nodiscard]] Int offset() const { return _offset; }

  void setOffset(Int offset) { _offset = offset; }
};

}  // namespace invariantgraph