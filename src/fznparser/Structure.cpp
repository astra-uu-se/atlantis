#include "Structure.h"

using namespace std;

Parameter::Parameter(string name) {
  _name = name;
};
Variable::Variable(string name) {
  _name = name;
};
Constraint::Constraint(string name) {
  _name = name;
};

Model::Model(vector<Parameter> parameters,
             vector<Variable> variables,
             vector<Constraint> constraints) {

  _parameters = parameters;
  _variables = variables;
  _constraints = constraints;
};
