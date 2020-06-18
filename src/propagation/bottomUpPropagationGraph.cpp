#include "propagation/bottomUpPropagationGraph.hpp"

BottomUpPropagationGraph::BottomUpPropagationGraph(Engine& e,
                                                   size_t expectedSize)
    : PropagationGraph(expectedSize), m_engine(e) {
  variableStack.reserve(expectedSize);
  invariantStack.reserve(expectedSize);
  isUpToDate.reserve(expectedSize);
  hasVisited.reserve(expectedSize);

  // add null entry.
  isUpToDate.push_back(false);
  hasVisited.push_back(false);
}

void BottomUpPropagationGraph::notifyMaybeChanged(const Timestamp& t,
                                                  VarId id) {
  assert(false);  // TODO: we do not need to crash here, but note that we do not
                  // need to notify of any change!
}

void BottomUpPropagationGraph::clearForQuery() {
  variableStack.clear();
  invariantStack.clear();
  isUpToDate.assign(isUpToDate.size(), false);
  hasVisited.assign(hasVisited.size(), false);
}
void BottomUpPropagationGraph::registerForQuery(VarId id) {
  variableStack.push_back(id);
}
void BottomUpPropagationGraph::propagate() {
  // recursively expand variables to compute their value.
  while (!variableStack.empty()) {
    VarId currentVar = variableStack.back();
    if (hasVisited.at(currentVar)) {
      assert(false);
      // TODO: Maybe a dynamic cycle! throw exception: illegal move or illegal
      // initial assignment.
      // TODO: However, if we query for a two variables and one depend on the
      // other, then we will also reach this case. But then it is not a cycle
      // and we can just ignore it. Maybe the rule is: if we reach a visited AND
      // there are invariants on the stack, then we have a dynamic cycle.
    }
    setVisited(currentVar);
    // If the variable has not been expanded before, then expand it.
    if (!isUpToDate.at(currentVar)) {
      if (m_definingInvariant.at(currentVar).id != NULL_ID) {
        // Variable is defined, so expand defining invariant.
        expandInvariant(m_definingInvariant.at(currentVar));
        continue;
      } else if (m_engine.hasChanged(m_engine.getCurrentTime(), currentVar)) {
        // variable is not defined, so if it has changed, then we notify the top
        // invariant.
        // Note that this must be an input variable.
        // TODO: just pre-mark all the input variables and remove this case!
        notifyCurrentInvariant(currentVar);
        nextVar();
        continue;
      } else {
        continue;
      }
    } else if (invariantStack.empty()) {
      nextVar();  // we are at an output variable!
    } else {
      if (m_engine.hasChanged(m_engine.getCurrentTime(), currentVar)) {
        // Else, if the variable has been marked before and has changed then
        // just send a notification to top invariant (i.e, the one asking for
        // its value)
        notifyCurrentInvariant(currentVar);
        nextVar();
        continue;
      } else {
        // pop
        nextVar();
        continue;
      }
    }
  }
}

// VarId BottomUpPropagationGraph::getNextStableVariable(const Timestamp& t) {
// }

void BottomUpPropagationGraph::registerVar(VarId id) {
  PropagationGraph::registerVar(id);  // call parent implementation
  variableStack.push_back(NULL_ID);   // push back just to resize the stack!
  isUpToDate.push_back(false);
  hasVisited.push_back(false);
}

void BottomUpPropagationGraph::registerInvariant(InvariantId id) {
  PropagationGraph::registerInvariant(id);  // call parent implementation
  invariantStack.push_back(NULL_ID);  // push back just to resize the stack!
}