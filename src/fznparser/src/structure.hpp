#pragma once
#include <cassert>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

class Constraint;
class Model;
class Item;

class Domain {
 public:
  Domain() = default;
  ~Domain() = default;
  virtual int size();
  virtual int lowerBound() = 0;
  virtual int upperBound() = 0;

 protected:
  bool _defined;
};

class BoolDomain : public Domain {
 public:
  BoolDomain() = default;
  int size() override;
  int lowerBound() override;
  int upperBound() override;
};

class IntSetDomain : public Domain {
 public:
  IntSetDomain(std::set<int> set);
  int size() override;
  int lowerBound() override;
  int upperBound() override;

 private:
  std::set<int> _set;
};

class IntDomain : public Domain {
 public:
  IntDomain();
  IntDomain(int lb, int ub);
  int size() override;
  int lowerBound() override;
  int upperBound() override;

 private:
  int _lb;
  int _ub;
};

class Annotation {
 public:
  Annotation();
  Annotation(std::string name, std::string variableName);
  bool definesVar();
  std::string _name;
  std::string _variableName;
  bool _definesVar;
};

class Expression {
 public:
  Expression();
  Expression(std::string name, bool isId);
  Expression(std::string name, std::vector<Expression>, bool isId);
  bool isId() { return _isId; }
  bool isArray() { return _isArray; }
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
  virtual std::set<Node*> getNext() = 0;
  virtual std::string getLabel() = 0;
  virtual bool breakCycle() = 0;
};