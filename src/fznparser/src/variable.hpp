#pragma once

#include "structure.hpp"

class Variable : public Node {
 public:
  Variable() = default;
  Variable(std::string name, std::vector<Annotation> annotations);
  virtual ~Variable() = default;
  virtual void init(
      std::map<std::string, std::shared_ptr<Variable>>& variables) = 0;

  virtual void addConstraint(Node* constraint) = 0;
  virtual void removeConstraint(Node* constraint) = 0;
  virtual void defineBy(Node* constraint) = 0;
  virtual void removeDefinition() = 0;
  virtual void addPotentialDefiner(Constraint* constraint) = 0;

  virtual bool isDefined() { return _isDefined; };
  virtual bool isDefinable() { return _isDefinable; };
  std::set<Node*> getNext() = 0;
  virtual std::string getName() { return _name; };
  virtual std::string getLabel() { return _name; };

  std::string _name;
  bool _isDefinable;
  bool _isDefined;
  Node* _definedBy;
  std::vector<Annotation> _annotations;
  std::set<Node*> _nextConstraints;
  std::set<Constraint*> _potentialDefiners;
};

class SingleVariable : public Variable {
 public:
  SingleVariable() = default;
  SingleVariable(std::string name, std::vector<Annotation> annotations,
                 std::shared_ptr<Domain> domain)
      : Variable(name, annotations) {
    _domain = domain;
  };
  void init(
      std::map<std::string, std::shared_ptr<Variable>>& variables) override{};

  std::set<Node*> getNext() override { return _nextConstraints; };
  void addConstraint(Node* constraint) override;
  void removeConstraint(Node* constraint) override;
  void defineBy(Node* constraint) override;
  void removeDefinition() override;
  void addPotentialDefiner(Constraint* constraint) override;

  std::shared_ptr<Domain> _domain;
};

class ArrayVariable : public Variable {
 public:
  ArrayVariable(std::string name, std::vector<Annotation> annotations,
                std::vector<Expression> expressions)
      : Variable(name, annotations) {
    _expressions = expressions;
  };
  void init(
      std::map<std::string, std::shared_ptr<Variable>>& variables) override;

  std::set<Node*> getNext() override;
  void addConstraint(Node* constraint) override;
  void removeConstraint(Node* constraint) override;
  void defineBy(Node* constraint) override;
  void removeDefinition() override;
  void addPotentialDefiner(Constraint* constraint) override;

  std::vector<Expression> _expressions;
  std::vector<Variable*> _elements;
};

class Parameter : public SingleVariable {
 public:
  Parameter(std::string value);
  void init(
      std::map<std::string, std::shared_ptr<Variable>>& variables) override;

  std::set<Node*> getNext() override;
  bool isDefinable() override { return false; };
  void addConstraint(Node* constraint) override{};
  void removeConstraint(Node* constraint) override{};
  void defineBy(Node* constraint) override{};
  void removeDefinition() override{};
  void addPotentialDefiner(Constraint* constraint) override{};
};
