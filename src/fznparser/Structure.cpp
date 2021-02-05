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

Variable::Variable(string name, std::shared_ptr<Domain> domain,
                   vector<Annotation> annotations) {
  _name = name;
  _domain = domain;
  _annotations = annotations;
};

Constraint::Constraint(string name) { _name = name; };

Model::Model(std::vector<std::shared_ptr<Variable>> variables,
             std::vector<std::shared_ptr<Constraint>> constraints) {
  _variables = variables;
  _constraints = constraints;
};
