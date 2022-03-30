#include "search/searchProcedure.hpp"

#include "misc/logging.hpp"

std::ostream& operator<<(std::ostream& os, const search::Cost& cost) {
  os << "(" << cost.violationDegree() << ", " << cost.objectiveValue() << ")";
  return os;
}

void search::SearchProcedure::run(SearchController& controller,
                                  SolutionListener& solutionListener,
                                  search::Annealer& annealer) {
  do {
    _assignment.assign([&](auto& modifications) {
      _neighbourhood.initialise(_random, modifications);
    });

    // For some reason, this does not compile, even though we define the
    // operator<< for std::ostream and search::Cost:
    // logDebug("Initialised assignment with cost " << _assignment.cost());

    // Once figured out how to do this, the debug call is expanded manually:
#ifndef NDDEBUG
    if (Logging::globalLogLevel >= Logging::LogLevel::debug) {
      std::cerr << "[debug] " << __FILE__ << " (" << __LINE__ << "): "
                << "Initialised assignment with cost " << _assignment.cost()
                << std::endl;
    }
#endif

    while (controller.shouldRun(_assignment) && annealer.hasNextLocal()) {
      while (controller.shouldRun(_assignment) && annealer.exploreLocal()) {
        bool madeMove =
            _neighbourhood.randomMove(_random, _assignment, annealer);

        if (madeMove) {
          //          logDebug("Committed assignment. New cost: " <<
          //          _assignment.cost());
#ifndef NDDEBUG
          if (Logging::globalLogLevel >= Logging::LogLevel::debug) {
            std::cerr << "[debug] " << __FILE__ << " (" << __LINE__ << "): "
                      << "Committed a move. New cost: " << _assignment.cost()
                      << std::endl;
          }
#endif
        }

        if (madeMove && _assignment.satisfiesConstraints()) {
          solutionListener.onSolution(_assignment);
        }
      }

      annealer.nextLocal();
    }
  } while (controller.shouldRun(_assignment));
}
