#include <string>
#include <vector>

using namespace std;
enum Type {intVar, boolVar};

class Domain {
  public:
    Domain(int lb, int ub);
    Domain();

    bool _undefined;
    Type _type;
    int _lb;
    int _ub;
};

class Annotation {
  public:
    Annotation();
};

class Variable {
  public:
    Variable(string name, Domain domain, vector<Annotation> annotations);

    string _name;
    vector<Annotation> _annotations;
    Domain _domain;

};



class Constraint {
  public:
    Constraint(string name);

    string _name;
    vector<Variable*> _varRefs;
};


class Model {
  public:
    Model(vector<Variable> variables,
          vector<Constraint> constraints);

    vector<Variable> _variables;
    vector<Constraint> _constraints;
};
