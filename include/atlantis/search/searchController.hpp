#pragma once

#include <chrono>
#include <iostream>

#include "assignment.hpp"
#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/utils/fznAst.hpp"
#include "atlantis/utils/fznOutput.hpp"
#include "fznparser/model.hpp"

namespace atlantis::search {

class SearchController {
 private:
  std::optional<std::chrono::milliseconds> _timeout;
  std::vector<FznOutputVar> _outputBoolVars;
  std::vector<FznOutputVar> _outputIntVars;
  std::vector<FznOutputVarArray> _outputBoolVarArrays;
  std::vector<FznOutputVarArray> _outputIntVarArrays;

  std::chrono::steady_clock::time_point _startTime;
  bool _isSatisfactionProblem;
  bool _started{false};
  Int _foundSolution{false};

 public:
  SearchController(const fznparser::Model& model,
                   const invariantgraph::FznInvariantGraph& invariantGraph)
      : _timeout({}),
        _outputBoolVars(invariantGraph.outputBoolVars()),
        _outputIntVars(invariantGraph.outputIntVars()),
        _outputBoolVarArrays(invariantGraph.outputBoolVarArrays()),
        _outputIntVarArrays(invariantGraph.outputIntVarArrays()),
        _isSatisfactionProblem(model.isSatisfactionProblem()) {}

  template <typename Rep, typename Period>
  SearchController(const fznparser::Model& model,
                   const invariantgraph::FznInvariantGraph& invariantGraph,
                   std::chrono::duration<Rep, Period> timeout)
      : _timeout(
            std::chrono::duration_cast<std::chrono::milliseconds>(timeout)),
        _outputBoolVars(invariantGraph.outputBoolVars()),
        _outputIntVars(invariantGraph.outputIntVars()),
        _outputBoolVarArrays(invariantGraph.outputBoolVarArrays()),
        _outputIntVarArrays(invariantGraph.outputIntVarArrays()),
        _isSatisfactionProblem(model.isSatisfactionProblem()) {}

  bool shouldRun(const Assignment&);
  static void printBoolVar(const Assignment&, const FznOutputVar&);
  static void printIntVar(const Assignment&, const FznOutputVar&);
  static void printBoolVarArray(const Assignment&, const FznOutputVarArray&);
  static void printIntVarArray(const Assignment&, const FznOutputVarArray&);
  void printSolution(const Assignment&) const;
  void onSolution(const Assignment&);
  void onFinish() const;
};

}  // namespace atlantis::search
