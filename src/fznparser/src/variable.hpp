#pragma once

#include "structure.hpp"

class Variable : public Node {
 public:
  Variable() = default;
  Variable(std::string name, std::vector<Annotation> annotations);
  virtual ~Variable() = default;

  virtual void addConstraint(Node* constraint) = 0;
  std::vector<Node*> getNext() = 0;
  virtual std::string getName() { return _name; };
  virtual std::string getLabel() { return _name; };
  virtual bool isDefinable() = 0;
  virtual void init(
      std::map<std::string, std::shared_ptr<Variable>>& variables) = 0;

  std::string _name;
  bool _isDefinable;
  bool _isDefined;
  Node* _definedBy;
  std::vector<Annotation> _annotations;
  void defineBy(Node* constraint);
  std::vector<Node*> _nextConstraints;
};

class SingleVariable : public Variable {
 public:
  SingleVariable() = default;
  SingleVariable(std::string name, std::vector<Annotation> annotations,
                 std::shared_ptr<Domain> domain)
      : Variable(name, annotations) {
    _domain = domain;
  };

  std::vector<Node*> getNext() override;
  bool isDefinable() override { return _isDefinable; };
  void addConstraint(Node* constraint) override;
  bool isDefined();
  void init(
      std::map<std::string, std::shared_ptr<Variable>>& variables) override;

  std::shared_ptr<Domain> _domain;
};

class ArrayVariable : public Variable {
 public:
  ArrayVariable(std::string name, std::vector<Annotation> annotations,
                std::vector<Expression> expressions)
      : Variable(name, annotations) {
    _expressions = expressions;
  };
  ArrayVariable(std::string name, std::vector<Expression> expressions);
  bool isDefinable() override { return false; };
  void init(
      std::map<std::string, std::shared_ptr<Variable>>& variables) override;
  std::vector<Node*> getNext() override;
  void addConstraint(Node* constraint) override;
  std::vector<Expression> _expressions;
  std::vector<Variable*> _elements;
};

class Parameter : public SingleVariable {
 public:
  Parameter(std::string value);
  void init(
      std::map<std::string, std::shared_ptr<Variable>>& variables) override;

  std::vector<Node*> getNext() override;
  std::string getName() override;
  std::string getLabel() override;
  bool isDefinable() override;
  void addConstraint(Node* constraint) override;

  std::string _value;
};
