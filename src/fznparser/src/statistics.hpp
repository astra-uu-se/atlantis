#ifndef __STATISTICS_HPP_INCLUDED__
#define __STATISTICS_HPP_INCLUDED__
#include "model.hpp"

class Statistics {
 public:
  Statistics(Model* model);
  std::string header();
  std::string row(int i);
  std::string line();
  std::string count();
  void countDefinedVariables(bool labels);
  void variablesDefinedBy();
  void allStats(bool labels);
  void constraints(bool labels);
  int countSoft();
  int variableCount();
  int definedCount();
  int annotationCount();
  int width(bool labels);
  void width_aux(std::vector<Node*>& result, std::vector<Node*> visited,
                 Node* node, int x, int& w);
  void matchingAnnotations(bool labels);

  double score();
  std::optional<double> matchingAnnotationsScore();
  std::optional<double> variableScore();
  std::optional<double> constraintScore();

  Model* _model;
};

#endif
