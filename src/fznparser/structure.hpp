#pragma once
#include <memory>
#include <set>
#include <string>
#include <vector>


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
  bool _isId;
  std::string _name;
};

class Node {
 public:
  virtual std::vector<std::shared_ptr<Node>> getNext() = 0;
    virtual ~Node() = default;
};

class Variable : public Node {
 public:
  Variable(std::string name, std::shared_ptr<Domain> domain,
           std::vector<Annotation> annotations);
  std::vector<std::shared_ptr<Node>> getNext() override;

  std::vector<std::shared_ptr<Node>> _constraints;
  std::string _name;
  std::shared_ptr<Domain> _domain;
  std::vector<Annotation> _annotations;
};
