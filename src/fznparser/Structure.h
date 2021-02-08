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
  Constraint(std::string name);
  virtual void print();


  std::string _name;
  std::vector<Variable*> _varRefs;
  std::vector<Annotation> _annotations;
};

class IntDiv : public Constraint {
 public:
  IntDiv(Variable* a, Variable* b, Variable* c,
         std::vector<Annotation> annotations);
  void print() override;
  // Figure out how to get the reference here if we create this inside visitor.
  // Raw pointers, rimligt här? Pga cykler.
  // Kolla upp optional klass
  Variable* _a;
  Variable* _b;
  Variable* _c;
  std::vector<Annotation> _annotations;
};

class Model {
 public:
  Model(std::map<std::string, std::shared_ptr<Variable>> variables,
        std::vector<std::shared_ptr<Constraint>> constraints);
  std::map<std::string, std::shared_ptr<Variable>> _variables;
  std::vector<std::shared_ptr<Constraint>> _constraints;
  static std::shared_ptr<Constraint> createConstraint(
      ConstraintItem ci,
      std::map<std::string, std::shared_ptr<Variable>> variables);
};
