#include "Structure.h"

using namespace std;

Domain::Domain() { _defined = false; }
bool Domain::defined() { return _defined; }
IntDomain::IntDomain() { _defined = false; }
IntDomain::IntDomain(int lb, int ub) {
  _defined = true;
  _lb = lb;
  _ub = ub;
}
IntDomain::IntDomain(std::set<int> set) {
  _defined = true;
  _set = set;
  _lb = *(set.begin());
  _ub = *(set.rbegin());
}
int IntDomain::getLb() { return _lb; }
int IntDomain::getUb() { return _ub; }
BoolDomain::BoolDomain() { _defined = false; }
int BoolDomain::getLb() { return false; }
int BoolDomain::getUb() { return true; }

Annotation::Annotation() {}
Annotation::Annotation(std::string name) { _name = name; }

Expression::Expression() {}
Expression::Expression(std::string name, bool isId) {
  _name = name;
  _isId = isId;
}

Variable::Variable(std::string name, std::shared_ptr<Domain> domain,
                   vector<Annotation> annotations) {
  _name = name;
  _domain = domain;
  _annotations = annotations;
};

ConstraintItem::ConstraintItem() {}
ConstraintItem::ConstraintItem(std::string name,
                               std::vector<Expression> expressions,
                               std::vector<Annotation> annotations) {
  _name = name;
  _expressions = expressions;
  _annotations = annotations;
}

Constraint::Constraint(string name) { _name = name; };
Constraint::Constraint(){};
void Constraint::print() {std::cout << "Nothing to print\n";}

IntDiv::IntDiv(Variable* a, Variable* b, Variable* c, std::vector<Annotation> annotations) {
  _a = a;
  _b = b;
  _c = c;
  _annotations = annotations;
}
void IntDiv::print() {
  std::cout << _a->_name << std::endl;
  std::cout << _b->_name << std::endl;
  std::cout << _c->_name << std::endl;
}

Model::Model(std::map<std::string, std::shared_ptr<Variable>> variables,
             std::vector<std::shared_ptr<Constraint>> constraints) {
  _variables = variables;
  _constraints = constraints;
};

std::shared_ptr<Constraint> Model::createConstraint(
    ConstraintItem ci,
    std::map<std::string, std::shared_ptr<Variable>> variables) {
  if (ci._name == "int_div") {
    Variable* a = (variables.find(ci._expressions[0]._name))->second.get();
    Variable* b = (variables.find(ci._expressions[1]._name))->second.get();
    Variable* c = (variables.find(ci._expressions[2]._name))->second.get();
    return std::make_shared<IntDiv>(a, b, c, ci._annotations);
  }

  return std::make_shared<Constraint>();
}
