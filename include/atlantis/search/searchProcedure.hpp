#pragma once

#include "atlantis/search/objective.hpp"
#include "atlantis/search/searchStatistics.hpp"
#include "savedAssignment.hpp"
#include "threadController.hpp"

namespace atlantis::logging {
class Logger;
}

namespace atlantis::search {

class MetaHeuristic;
class RandomProvider;
class Assignment;
class SearchController;
namespace neighborhoods {
class Neighborhood;
}

enum class SearchType : unsigned char { PARALLEL, BESTCOST, BEAMSEARCH };
constexpr std::array<std::string_view, 3> searchTypeNames = {
    "parallel", "cost-sharing", "beam"};

/**
 * Search procedure based on chapter 12 of:
 *
 * P. Van Hentenryck and L. Michel. Constraint-Based Local Search. The MIT
 * Press, 2005.
 */
class SearchProcedure {
  RandomProvider& _random;
  Assignment& _assignment;
  std::shared_ptr<neighborhoods::Neighborhood> _neighborhood;
  std::optional<SavedAssignment> _localBestAssignment;
  bool _hasSolution = false;
  const SearchType _searchType;

  // TODO: these things should ideally be abstracted from the search itself.
  const std::shared_ptr<ThreadController> _threadController;
  const std::vector<propagation::VarViewId> _outputVarIds;
  const Int _threadId;

  [[nodiscard]] SavedAssignment saveAssignment() const;

  // Returns true iff the was communication to other threads.
  bool onAccepted(const std::shared_ptr<CounterStatistic>& improvingSolutions,
                  std::unique_ptr<MetaHeuristic>&& metaHeuristic);

 public:
  SearchProcedure(
      RandomProvider& random, Assignment& assignment,
      const std::shared_ptr<neighborhoods::Neighborhood>& neighborhood,
      const SearchType searchType,
      const std::shared_ptr<ThreadController>& threadController,
      const std::vector<propagation::VarViewId>& outputVarIds,
      const Int threadId)
      : _random(random),
        _assignment(assignment),
        _neighborhood(neighborhood),
        _searchType(searchType),
        _threadController(threadController),
        _outputVarIds(outputVarIds),
        _threadId(threadId) {}

  Int run(SearchController&, std::unique_ptr<MetaHeuristic>&&);
};

}  // namespace atlantis::search
