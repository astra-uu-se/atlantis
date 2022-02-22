#pragma once

#include <map>

#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "fznparser/variable.hpp"
#include "search/assignment.hpp"

/**
 * Outputs information in the format specified by MiniZinc.
 *
 * @see https://www.minizinc.org/doc-2.5.5/en/fzn-spec.html#output
 */
class MiniZincLogger {
 private:
  const std::map<std::shared_ptr<fznparser::Variable>, VarId>& _variableMap;
  bool _loggedSolution = false;

 public:
  /**
   * Create a new MiniZinc logger.
   *
   * @param variableMap The output variables for which to log values when
   * a solution is found. All the keys must be variables that are annotated
   * with a 'output_var' or 'output_array' annotation.
   */
  explicit MiniZincLogger(
      const std::map<std::shared_ptr<fznparser::Variable>, VarId>& variableMap)
      : _variableMap(variableMap) {}

  /**
   * An implementation must output values for all and only the variables
   * annotated with output_var or output_array (output annotations must not
   * appear on parameters). Output must be printed to the standard output
   * stream.
   *
   * @param engine The assignment which is a solution.
   */
  void solution(const search::Assignment& assignment);

  enum class FinishReason { Exhausted, Terminated };

  /**
   * Indicate that the search has finished. Depending on whether the search
   * was exhaustive or the search was terminated for another reason, the output
   * should differ.
   *
   * @param reason The reason that the search finished.
   */
  void finish(FinishReason reason);
};