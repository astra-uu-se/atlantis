#pragma once

#include "model.hpp"

class Statistics {
 public:
  Statistics() = default;
  Statistics(Model* model, bool ignoreDynamicCycles, bool noWidth) {
    _model = model;
    _ignoreDynamicCycles = ignoreDynamicCycles;
    _noWidth = noWidth;
  };
  std::string header();
  std::string row();
  std::string line();
  std::string count();
  std::string info();
  std::string latexHeader();
  std::string latexRow();
  std::string latexCount();

  void countDefinedVariables(bool labels);
  void variablesDefinedBy();
  void freeVariables();
  void enlDomVariables();
  void allStats(bool labels);
  void constraints(bool labels);
  int countImplicit();
  int countPotImplicit();
  int countSoft();
  int countBadSoft();
  int countOnlySoft();
  int variableCount();
  int noPotDefCount();
  int definedCount();
  int annotationCount();
  int enlargedDomains();
  int enlargedDomainSum();
  int height();
  void height_aux(std::map<Node*, int>& visited, Node* node, int x, int& h);
  void matchingAnnotations(bool labels);

  std::optional<double> matchingAnnotationsScore();
  std::optional<double> variableScore();
  std::optional<double> constraintScore();

  std::vector<Variable*> outputVars();
  std::vector<Variable*> inputVars();
  int neighbourhoodSize();

  Model* _model;
  bool _ignoreDynamicCycles;
  bool _noWidth;
};
