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
  bool definesVar();
  std::string _name;
  bool _definesVar;
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
  virtual std::set<Node*> getNext() = 0;
  virtual std::string getLabel() = 0;
};
