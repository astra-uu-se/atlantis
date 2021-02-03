#include "Structure.h"

using namespace std;

Domain::Domain(int lb, int ub) {
  _undefined = false;
  _lb = lb;
  _ub = ub;
};
Domain::Domain() {
  _undefined = true;
  _lb = 0;
  _ub = 0;
}

Annotation::Annotation() {
 
}


Variable::Variable(string name, Domain domain, vector<Annotation> annotations) {
  _name = name;
  _domain = domain;
  _annotations = annotations;
};

Constraint::Constraint(string name) {
  _name = name;
};

Model::Model(vector<Variable> variables,
             vector<Constraint> constraints) {

  _variables = variables;
  _constraints = constraints;
};
