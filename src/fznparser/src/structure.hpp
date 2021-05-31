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
using Int = int64_t;

class Domain {
 public:
  Domain() = default;
  ~Domain() = default;
  virtual Int size();
  virtual Int lowerBound() = 0;
  virtual Int upperBound() = 0;
  virtual bool isContinuous() { return true; };

 protected:
  bool _defined;
};

class BoolDomain : public Domain {
 public:
  BoolDomain() = default;
  Int size() override;
  Int lowerBound() override;
  Int upperBound() override;
};

class IntSetDomain : public Domain {
 public:
  IntSetDomain(std::set<Int> set);
  Int size() override;
  Int lowerBound() override;
  Int upperBound() override;
  bool isContinuous() override { return false; };

 private:
  std::set<Int> _set;
};

class IntDomain : public Domain {
 public:
  IntDomain();
  IntDomain(Int lb, Int ub);
  Int size() override;
  Int lowerBound() override;
  Int upperBound() override;
  void setLower(Int lb) { _lb = lb; }
  void setUpper(Int ub) { _ub = ub; }

 private:
  Int _lb;
  Int _ub;
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
  virtual std::vector<Node*> getNext() = 0;
  virtual std::string getName() = 0;
};
