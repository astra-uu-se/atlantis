#pragma once
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <cassert>

class Constraint;
class Model;
class Item;

class Domain {
 public:
  Domain();
  virtual int getLb() = 0;
  virtual int getUb() = 0;
  bool defined();
  bool _defined;
};

class BoolDomain : public Domain {
 public:
  BoolDomain();

  int getLb() override;
  int getUb() override;
};

class IntDomain : public Domain {
 public:
  IntDomain();
  IntDomain(std::set<int> set);
  IntDomain(int lb, int ub);

  int getLb() override;
  int getUb() override;

  int _lb;
  int _ub;
  std::set<int> _set;
};

class Annotation {
 public:
  Annotation();
  Annotation(std::string name);
  std::string _name;
};

class Expression {
 public:
  Expression();
  Expression(std::string name, bool isId);
  Expression(std::string name, std::vector<Expression>, bool isId);
  bool isId() { return _isId; };
  bool isArray() { return _isArray; };
  std::string getName();
  bool _isId;
  bool _isArray;
  std::string _name;
  std::vector<Expression> _elements;
};

class Node {
 public:
    Node() = default;
  virtual ~Node() = default;
  virtual std::vector<Node*> getNext() = 0;
  virtual std::string getLabel() = 0;
};

class Item : public Node {
 public:
    Item() = default;
    virtual ~Item() = default;
  virtual bool isDefinable() = 0;
  virtual std::string getName() = 0;
  virtual void init(std::map<std::string, std::shared_ptr<Item>>& items) = 0;
    virtual void addConstraint(Node* constraint) = 0;
};

class Variable : public Item {
 public:
    Variable() = default;
  Variable(std::string name, std::vector<Annotation> annotations);
    virtual ~Variable() = default;

  std::string _name;
  bool _isDefinable;
  bool _isDefined;
  Node* _definedBy;
  std::vector<Annotation> _annotations;
    void defineBy(Node* constraint);
  std::vector<Node*> _nextConstraints;
  std::vector<Node*> getNext() override;
  std::string getName() override { return _name; };
  std::string getLabel() override { return _name; };
};

class SingleVariable : public Variable {
 public:
    SingleVariable() = default;
  SingleVariable(std::string name, std::vector<Annotation> annotations,
                 std::shared_ptr<Domain> domain)
      : Variable(name, annotations) {
    _domain = domain;
  };
  bool isDefinable() override { return _isDefinable; };
  void addConstraint(Node* constraint) override;
  bool isDefined();
  void init(std::map<std::string, std::shared_ptr<Item>>& items) override;

  std::shared_ptr<Domain> _domain;
};

class ArrayVariable : public Variable {
 public:
  ArrayVariable(std::string name, std::vector<Annotation> annotations,
                std::vector<Expression> expressions)
      : Variable(name, annotations) {
    _expressions = expressions;
  };
  ArrayVariable(std::string name,
                std::vector<Expression> expressions);
  bool isDefinable() override { return false; };
  void init(std::map<std::string, std::shared_ptr<Item>>& items) override;
  std::vector<Node*> getNext() override;
  void addConstraint(Node* constraint) override;
  std::vector<Expression> _expressions;
  std::vector<Item*> _elements;
};

class Parameter : public SingleVariable {
 public:
  Parameter(std::string value);
  void init(std::map<std::string, std::shared_ptr<Item>>& items) override;

  std::vector<Node*> getNext() override;
  std::string getName() override;
  std::string getLabel() override;
  bool isDefinable() override;
  void addConstraint(Node* constraint) override;

  std::string _value;
};
