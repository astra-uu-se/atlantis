#include "statistics.hpp"

#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

int Statistics::variableCount() {
  int n = 0;
  for (auto variable : _model->varMap().variables()) {
    if (variable->isDefinable()) n++;
  }
  return n;
}
int Statistics::noPotDefCount() {
  int n = 0;
  for (auto variable : _model->varMap().variables()) {
    if (variable->isDefinable() && variable->noPotDef()) n++;
  }
  return n;
}

int Statistics::definedCount() {
  int n = 0;
  for (auto variable : _model->varMap().variables()) {
    if (variable->isDefined() && variable->isDefinable()) {
      n++;
    }
  }
  return n;
}
void Statistics::countDefinedVariables(bool labels) {
  std::cout << "===========================" << std::endl;
  std::cout << "=========VARIABLES=========" << std::endl;
  if (labels) {
    std::cout << "=========BAD=========" << std::endl;
    freeVariables();
    std::cout << "=========ENLARGED=========" << std::endl;
    enlDomVariables();
  }
  std::cout << "===========================" << std::endl;
  std::cout << "Defined:\t";
  std::cout << definedCount() << std::endl;
  std::cout << "Total:\t\t";
  std::cout << variableCount() << std::endl;
  std::cout << "===========================" << std::endl;
}
void Statistics::variablesDefinedBy() {
  for (auto variable : _model->varMap().variables()) {
    if (variable->isDefined()) {
      std::cout << "Var: " << variable->getName();
      std::cout << "\tDS: " << variable->domainSize();
      if (variable->hasImposedDomain()) {
        std::cout << "\t(imposed)";
        if (variable->hasEnlargedDomain()) {
          std::cout << "(>)";
        }
      }
      std::cout << "\t[" << variable->definedBy()->getName() << "]"
                << std::endl;
    }
  }
}
void Statistics::freeVariables() {
  for (auto variable : _model->varMap().variables()) {
    if (!variable->isDefined() && variable->isDefinable() &&
        !variable->noPotDef()) {
      std::cout << "Var: " << variable->getName();
      std::cout << "\tDS: " << variable->domainSize();
      std::cout << "\tPot. Def:" << variable->orgPotDefSize();
      std::cout << std::endl;
    }
  }
}
void Statistics::enlDomVariables() {
  for (auto variable : _model->varMap().variables()) {
    if (variable->hasEnlargedDomain()) {
      std::cout << "Var: " << variable->getName();
      std::cout << "\tDS: " << variable->domainSize();
      std::cout << "\tORG: " << variable->_domain->size();
      std::cout << "\tPot. Def: " << variable->orgPotDefSize();
      std::cout << "\tDefined by: " << variable->definedBy()->getName();
      std::cout << std::endl;
    }
  }
}
int Statistics::countPotImplicit() {
  int imp = 0;
  for (auto constraint : _model->constraints()) {
    if (constraint->isPotImplicit()) imp++;
  }
  return imp;
}
int Statistics::countImplicit() {
  int imp = 0;
  for (auto constraint : _model->constraints()) {
    if (constraint->isImplicit()) imp++;
  }
  return imp;
}
int Statistics::countSoft() {
  int soft = 0;
  for (auto constraint : _model->constraints()) {
    if (constraint->isImplicit() || constraint->isInvariant()) {
      continue;
    }
    soft++;
  }
  return soft;
}
int Statistics::countOnlySoft() {
  int soft = 0;
  for (auto constraint : _model->constraints()) {
    if (constraint->isPotImplicit() && !constraint->onlyWrongAnnTarget()) {
      continue;
    }
    if (constraint->isFunctional()) {
      bool functional = false;
      for (auto var : constraint->variables()) {
        if (var->hasPotentialDefiner(constraint)) functional = true;
      }
      if (functional && !constraint->onlyWrongAnnTarget()) continue;
    }
    soft++;
  }
  return soft;
}

int Statistics::countBadSoft() {
  int soft = 0;
  for (auto constraint : _model->constraints()) {
    if (constraint->isImplicit() || constraint->isInvariant()) {
      continue;
    }
    if (constraint->isPotImplicit() && !constraint->onlyWrongAnnTarget()) {
      soft++;
    } else if (constraint->isFunctional()) {
      bool functional = false;
      for (auto var : constraint->variables()) {
        if (var->hasPotentialDefiner(constraint)) functional = true;
      }
      if (functional && !constraint->onlyWrongAnnTarget()) soft++;
    }
  }
  return soft;
}

void Statistics::constraints(bool labels) {
  std::cout << "========CONSTRAINTS========" << std::endl;
  int total = 0;
  int invariant = 0;
  int implicit = 0;
  for (auto constraint : _model->constraints()) {
    total++;
    if (constraint->isImplicit()) {
      implicit++;
    } else if (constraint->isInvariant()) {
      invariant++;
    }
    if (labels) {
      if (constraint->isImplicit()) {
        std::cout << constraint->getName() << std::endl;
      }
      if (constraint->isFunctional() && !constraint->isInvariant()) {
        bool functional = false;
        for (auto var : constraint->variables()) {
          if (var->hasPotentialDefiner(constraint)) functional = true;
        }
        if (functional && !constraint->onlyWrongAnnTarget()) {
          std::cout << constraint->getName() << std::endl;
        }
      }
    }
  }
  std::cout << "===========================" << std::endl;
  std::cout << "Invariants:\t" << invariant << std::endl;
  std::cout << "Implicit:\t" << implicit << std::endl;
  std::cout << "Soft:\t\t" << total - invariant - implicit << std::endl;
  std::cout << "Total:\t\t" << total << std::endl;
  std::cout << "===========================" << std::endl;
}
void Statistics::allStats(bool labels) {
  countDefinedVariables(labels);
  constraints(labels);
  matchingAnnotations(labels);
  std::cout << line() << std::endl;
}
void Statistics::matchingAnnotations(bool labels) {
  std::cout << "========ANNOTATIONS========" << std::endl;
  int annotations = 0;
  int matching = 0;
  for (auto constraint : _model->constraints()) {
    if (constraint->annotationTarget().has_value()) {
      annotations++;
      if (constraint->defines(constraint->annotationTarget().value())) {
        matching++;
      } else {
        if (labels) {
          std::cout << constraint->getName() << " should define "
                    << constraint->annotationTarget().value()->getName()
                    << " but doesn't." << std::endl;
        }
      }
    }
  }
  std::cout << "===========================" << std::endl;
  std::cout << "Annotations:\t" << annotations << std::endl;
  std::cout << "Matching:\t" << matching << std::endl;
  std::cout << "===========================" << std::endl;
}
int Statistics::height() {
  int h = 0;
  int x = 0;
  std::map<Node*, int> visited;
  for (Variable* v : _model->varMap().variables()) {
    if (!v->isDefined() && v->isDefinable()) {
      height_aux(visited, v, 1, h);
    }
  }
  for (Constraint* c : _model->constraints()) {
    if (c->isImplicit()) {
      height_aux(visited, c, 1, h);
    }
  }
  return h;
}

void Statistics::height_aux(std::map<Node*, int>& visited, Node* node, int x,
                            int& h) {
  if (auto var = dynamic_cast<Variable*>(node)) {
    if (!var->isDefinable()) {
      return;
    }
  }
  if (x > h) h = x;
  if (visited.find(node) == visited.end()) {
    visited.insert(std::pair<Node*, int>(node, x));
    for (auto n : node->getNext()) {
      height_aux(visited, n, x + 1, h);
    }
    return;
  }
  if (x > visited.find(node)->second) {
    visited.find(node)->second = x;
    for (auto n : node->getNext()) {
      height_aux(visited, n, x + 1, h);
    }
  }
}

int Statistics::annotationCount() {
  int annotations = 0;
  for (auto c : _model->constraints()) {
    if (c->annotationTarget().has_value()) {
      annotations++;
    }
  }
  return annotations;
}

std::optional<double> Statistics::matchingAnnotationsScore() {
  int annotations = 0;
  int matching = 0;
  for (auto constraint : _model->constraints()) {
    if (constraint->annotationTarget().has_value()) {
      annotations++;
      if (constraint->defines(constraint->annotationTarget().value()) &&
          constraint->isInvariant()) {
        matching++;
      }
    }
  }
  if (annotations > 0) {
    return 100 * matching / (double)annotations;
  }
  return {};
}
std::optional<double> Statistics::variableScore() {
  int vc = variableCount();
  vc -= noPotDefCount();
  if (vc > 0) {
    return 100 * definedCount() / (double)vc;
  }
  return {};
}
std::optional<double> Statistics::constraintScore() {
  if (_model->constraints().size() > 0) {
    return 100 *
           (_model->constraints().size() - countOnlySoft() - countBadSoft()) /
           ((double)_model->constraints().size() - countOnlySoft());
  }
  return {};
}
std::string Statistics::info() {
  std::stringstream s;
  s << line();
  s << "Information" << std::endl;
  s << line();
  s << "VarSco:\t\t";
  s << "Variable Score is the percentage of variables that are defined by an"
       "invariant or an implicit constraints."
    << std::endl;
  s << "InvSco:\t\t";
  s << "Invariant Score is the percentage of constraints that are either "
       "invariants or implicit constraints."
    << std::endl;
  s << "AnnSco:\t\t";
  s << "Annotation Score is the percentage of constraints with a defines_var "
       "annotation"
       "that define the targeted variable."
    << std::endl;
  s << "Height:\t\t";
  s << "Height is the length of the longest path in the graph."
       "(Not available when dynamic cycles are allowed.)"
    << std::endl;
  s << "In #:\t\t";
  s << "The amount of (definable) variables that are not defined." << std::endl;
  s << "Out #:\t\t";
  s << "The amount of (definable) variables that are defined and that are "
       "not "
       "input to another constraint."
    << std::endl;
  s << "Influence:\t";
  s << "Influence (average and max) is the percentage of variables that are "
       "children to an input variable."
    << std::endl;
  s << "Dependency:\t";
  s << "Dependency (average and max) is the percentage of variables that are "
       "ancestors to an output variable."
    << std::endl;
  s << "NbSum:\t\t";
  s << "The sum of the domains of the input variables." << std::endl;

  s << line();
  return s.str();
}
std::string Statistics::latexHeader() {
  std::stringstream s;
  s << "Scheme & VarSco & InvSco & AnnSco & Imp & Height & Free # & EnlDom "
       "(Sum)\\\\"
    << std::endl
    << "\\hline" << std::endl;
  return s.str();
}
std::string Statistics::header() {
  std::stringstream s;
  s << "Scheme\t\tVarSco\tInvSco\tAnnSco\tImp\tHeight\tFree #\t EnlDom (Sum)"
    << std::endl;
  line();
  return s.str();
}

std::string Statistics::line() {
  std::stringstream s;
  s << "---------------------------------------------------------------------"
    << std::endl;
  return s.str();
}
std::string Statistics::latexCount() {
  std::stringstream s;
  s << "Vars: "
    << " & " << variableCount() << " & "
    << "Definable: "
    << " & " << variableCount() - noPotDefCount() << " & "
    << "Cons: "
    << " & " << _model->constraints().size() << " & "
    << "Anns: "
    << " & " << annotationCount() << "& & & " << std::endl;
  return s.str();
}
std::string Statistics::count() {
  std::stringstream s;
  s << "Variables: " << variableCount() << std::endl
    << "Definable: " << variableCount() - noPotDefCount() << std::endl
    << "Constraints: " << _model->constraints().size() << std::endl
    << "Annotations: " << annotationCount() << std::endl;
  return s.str();
}
std::string Statistics::latexRow() {
  std::stringstream s;
  s << " & ";
  if (variableScore()) {
    s << std::fixed << std::setprecision(2) << variableScore().value() << " & ";
  } else {
    s << "- & ";
  }
  if (constraintScore()) {
    s << std::fixed << std::setprecision(2) << constraintScore().value()
      << " & ";
  } else {
    s << "- & ";
  }
  if (matchingAnnotationsScore()) {
    s << std::fixed << std::setprecision(2)
      << matchingAnnotationsScore().value() << " & ";
  } else {
    s << "- & ";
  }
  s << countImplicit() << "/" << countPotImplicit() << " & ";
  if (!_ignoreDynamicCycles && !_noWidth) {
    s << height();
  } else {
    s << "-";
  }
  s << " & " << std::fixed << std::setprecision(2) << inputVars().size();
  s << " & " << enlargedDomains() << " (" << enlargedDomainSum() << ")";
  s << "\\\\";

  s << std::endl;
  return s.str();
}
std::string Statistics::row() {
  std::stringstream s;
  s << "\t";
  if (variableScore()) {
    s << std::fixed << std::setprecision(2) << variableScore().value() << "\t";
  } else {
    s << "-\t";
  }
  if (constraintScore()) {
    s << std::fixed << std::setprecision(2) << constraintScore().value()
      << "\t";
  } else {
    s << "-\t";
  }
  if (matchingAnnotationsScore()) {
    s << std::fixed << std::setprecision(2)
      << matchingAnnotationsScore().value() << "\t";
  } else {
    s << "-\t";
  }
  s << countImplicit() << "/" << countPotImplicit() << "\t";
  if (!_ignoreDynamicCycles && !_noWidth) {
    s << height();
  } else {
    s << "-";
  }
  s << "\t" << std::fixed << std::setprecision(2) << inputVars().size();
  s << "\t" << enlargedDomains() << " (" << enlargedDomainSum() << ")";

  s << std::endl;
  return s.str();
}
std::vector<Variable*> Statistics::inputVars() {
  std::vector<Variable*> inputVars;
  for (auto var : _model->varMap().variables()) {
    if (!var->isDefined() && var->isDefinable()) {
      inputVars.push_back(var);
    }
  }
  return inputVars;
}
std::vector<Variable*> Statistics::outputVars() {
  std::vector<Variable*> outputVars;
  for (auto var : _model->varMap().variables()) {
    if (var->isDefined() && var->getNext().empty()) {
      outputVars.push_back(var);
    }
  }
  return outputVars;
}
int Statistics::neighbourhoodSize() {
  int nbhd = 0;
  for (auto var : inputVars()) {
    nbhd += var->domainSize();
  }
  return nbhd;
}

int Statistics::enlargedDomains() {
  int count = 0;
  for (auto var : _model->varMap().variables()) {
    if (var->hasEnlargedDomain()) {
      count++;
    }
  }
  return count;
}
int Statistics::enlargedDomainSum() {
  int sum = 0;
  for (auto var : _model->varMap().variables()) {
    if (var->hasEnlargedDomain()) {
      sum += var->_imposedDomain.value()->size() - var->_domain->size();
    }
  }
  return sum;
}
