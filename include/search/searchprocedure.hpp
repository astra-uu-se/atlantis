#pragma once

#include <map>
#include <memory>

#include "annealer.hpp"
#include "assignment.hpp"
#include "fznparser/model.hpp"
#include "neighbourhoods/neighbourhood.hpp"
#include "search/neighbourhoods/maxViolating.hpp"

namespace search {

class SearchContext {
 private:
  size_t _maxAttempts;
  size_t _attempts = 0;

 public:
  explicit SearchContext(size_t maxAttempts) : _maxAttempts(maxAttempts) {}

  bool shouldStop(const Assignment& assignment) {
    ++_attempts;

    return _attempts >= _maxAttempts || assignment.satisfiesConstraints();
  }
};

template <typename Neighbourhood>
class SearchProcedure {
  static_assert(
      std::is_base_of_v<neighbourhoods::Neighbourhood, Neighbourhood>);

 private:
  Annealer _annealer;
  Neighbourhood& _neighbourhood;
  Assignment _assignment;

  Int _violationsWeight = 1;
  Int _modelObjectiveWeight = 0;

 public:
  SearchProcedure(Annealer annealer, Neighbourhood& neighbourhood,
                  Assignment assignment)
      : _annealer(annealer),
        _neighbourhood(neighbourhood),
        _assignment(assignment) {}

  void run(
      SearchContext& context,
      const std::map<VarId, std::shared_ptr<fznparser::Variable>>& variableMap);

 private:
  bool accept(Move m);

  [[nodiscard]] Int evaluate(Objective objective) const;
};

typedef SearchProcedure<neighbourhoods::MaxViolatingNeighbourhood>
    MaxViolatingSearch;

}  // namespace search