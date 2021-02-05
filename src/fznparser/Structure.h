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
};

// Klass för arrayVariable eller bara array av Variables?
// Ska Variables ha referenser till constraints?
class Variable {
 public:
  Variable(std::string name, std::shared_ptr<Domain> domain,
           std::vector<Annotation> annotations);

  std::string _name;
  std::shared_ptr<Domain> _domain;
  std::vector<Annotation> _annotations;
};

// Smart Pointers
class Constraint {
 public:
  Constraint(std::string name);

  std::string _name;
  std::vector<Variable*> _varRefs;
  std::vector<Annotation> _annotations;
};

class IntDiv : public Constraint {
 public:
  // Figure out how to get the reference here if we create this inside visitor.
  // Raw pointers, rimligt här? Pga cykler.
  // Kolla upp optional klass
  Variable* a;
  Variable* b;
  Variable* c;
};

class Model {
 public:
  Model(std::vector<std::shared_ptr<Variable>> variables,
        std::vector<std::shared_ptr<Constraint>> constraints);
  std::vector<std::shared_ptr<Variable>> _variables;
  std::vector<std::shared_ptr<Constraint>> _constraints;
};
