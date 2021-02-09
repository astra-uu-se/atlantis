#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
// Kolla sort och GlobalCardinality

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

// Klass för arrayVariable eller bara array av Variables?
// Ska Variables ha referenser till constraints?

class Expression {
 public:
  Expression();
  Expression(std::string name, bool isId);
  bool _isId;
  std::string _name;
};

class Variable {
 public:
  Variable(std::string name, std::shared_ptr<Domain> domain,
           std::vector<Annotation> annotations);

  std::string _name;
  std::shared_ptr<Domain> _domain;
  std::vector<Annotation> _annotations;
};

class ConstraintItem {
 public:
  ConstraintItem();
  ConstraintItem(std::string name, std::vector<Expression> expressions,
                 std::vector<Annotation> annotations);
  std::string _name;
  std::vector<Expression> _expressions;
  std::vector<Annotation> _annotations;
};

class Constraint {
 public:
  Constraint();
  virtual void print() = 0;
  virtual void init(
      std::map<std::string, std::shared_ptr<Variable>> variables) = 0;

  ConstraintItem _constraintItem;
  std::string _name;
};

class IntDiv : public Constraint {
 public:
  IntDiv(ConstraintItem constraintItem);
  void print() override;
  void init(
      std::map<std::string, std::shared_ptr<Variable>> variables) override;
  Variable* _a;
  Variable* _b;
  Variable* _c;
};

class Model {
 public:
  Model();
  std::map<std::string, std::shared_ptr<Variable>> _variables;
  std::vector<std::shared_ptr<Constraint>> _constraints;
  void init();
  void addVariable(std::shared_ptr<Variable> v);
  void addConstraint(ConstraintItem ci);
};
