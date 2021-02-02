#include <string>
#include <vector>

using namespace std;

class Parameter {
public:
  Parameter(string name);

  string _name;
private:
};

class Variable {
public:
  Variable(string name);

  string _name;
private:
};

class Constraint {
public:
  Constraint(string name);

  string _name;
private:
};


class Model {
public:
  Model(vector<Parameter> parameters,
        vector<Variable> variables,
        vector<Constraint> constraints);

  vector<Parameter> _parameters;
  vector<Variable> _variables;
  vector<Constraint> _constraints;
private:
};
