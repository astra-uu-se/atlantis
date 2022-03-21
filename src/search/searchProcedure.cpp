#include "search/searchProcedure.hpp"

void search::SearchProcedure::run(SearchController& controller,
                                  search::Annealer* annealer) {
  while (controller.shouldRun(_assignment)) {
    _assignment.assign([&](auto& modifications) {
      _neighbourhood->initialise(modifications);
    });

    while (controller.shouldRun(_assignment) && annealer->hasNextLocal()) {
      while (controller.shouldRun(_assignment) && annealer->exploreLocal()) {
        _neighbourhood->randomMove(_assignment, annealer);
      }

      annealer->nextLocal();
    }
  }
}
