#include "search/searchProcedure.hpp"

void search::SearchProcedure::run(SearchController& controller,
                                  SolutionListener& solutionListener,
                                  search::Annealer* annealer) {
  while (controller.shouldRun(_assignment)) {
    _assignment.assign([&](auto& modifications) {
      _neighbourhood->initialise(modifications);
    });

    while (controller.shouldRun(_assignment) && annealer->hasNextLocal()) {
      while (controller.shouldRun(_assignment) && annealer->exploreLocal()) {
        bool madeMove = _neighbourhood->randomMove(_assignment, annealer);
        if (madeMove && _assignment.satisfiesConstraints()) {
          solutionListener.onSolution(_assignment);
        }
      }

      annealer->nextLocal();
    }
  }
}
