#include "core/engine.hpp"

#include "exceptions/exceptions.hpp"

extern Id NULL_ID;

Engine::Engine()
    : m_currentTime(0),
      // m_intVars(),
      // m_invariants(),
      m_propGraph(*this, ESTIMATED_NUM_OBJECTS),
      m_isOpen(false),
      m_store(ESTIMATED_NUM_OBJECTS, NULL_ID) {
  m_dependentInvariantData.reserve(ESTIMATED_NUM_OBJECTS);
  m_dependentInvariantData.push_back({});
}

void Engine::open() { m_isOpen = true; }

void Engine::recomputeAndCommit() {
  for (auto iter = m_store.invariantBegin(); iter != m_store.invariantEnd();
       ++iter) {
    assert((*iter) != nullptr);
    (*iter)->recompute(m_currentTime, *this);
    (*iter)->commit(m_currentTime, *this);
  }
  for (auto iter = m_store.intVarBegin(); iter != m_store.intVarEnd(); ++iter) {
    iter->commit();
  }
}

void Engine::close() {
  m_isOpen = false;

  // compute initial values for variables and for (internal datastructure of)
  // invariants
  recomputeAndCommit();
}

//---------------------Notificaion/Modification---------------------
void Engine::notifyMaybeChanged([[maybe_unused]] const Timestamp& t, VarId id) {
  m_propGraph.notifyMaybeChanged(t, id);
}

//--------------------- Move semantics ---------------------
void Engine::beginMove() { ++m_currentTime; }

void Engine::endMove() {}

void Engine::setValue(const Timestamp& t, VarId& v, Int val) {
  m_store.getIntVar(v).setValue(t, val);
  notifyMaybeChanged(t, v);
}

void Engine::incValue(const Timestamp& t, VarId& v, Int inc) {
  m_store.getIntVar(v).incValue(t, inc);
  notifyMaybeChanged(t, v);
}

Int Engine::getValue(const Timestamp& t, VarId& v) {
  return m_store.getIntVar(v).getValue(t);
}

Int Engine::getCommitedValue(VarId& v) {
  return m_store.getIntVar(v).getCommittedValue();
}

Timestamp Engine::getTmpTimestamp(VarId& v) {
  return m_store.getIntVar(v).getTmpTimestamp();
}

void Engine::commit(VarId& v) {
  m_store.getIntVar(v).commit();
  // todo: do something else? like:
  // m_store.getIntVar(v).validate();
}

void Engine::commitIf(const Timestamp& t, VarId& v) {
  m_store.getIntVar(v).commitIf(t);
  // todo: do something else? like:
  // m_store.getIntVar(v).validate();
}

void Engine::commitValue(VarId& v, Int val) {
  m_store.getIntVar(v).commitValue(val);
  // todo: do something else? like:
  // m_store.getIntVar(v).validate();
}

void Engine::beginQuery() { m_propGraph.clearForPropagation(); }

void Engine::query(VarId id) {
  m_propGraph.registerForPropagation(m_currentTime, id);
}

void Engine::endQuery() {
  // m_propGraph.schedulePropagation(m_currentTime, *this);
  // propagate();
  m_propGraph.propagate();
}

// Propagates at the current internal time of the engine.
void Engine::propagate() {
  VarId id = m_propGraph.getNextStableVariable(m_currentTime);
  while (id.id != NULL_ID) {
    IntVar& variable = m_store.getIntVar(id);
    if (variable.hasChanged(m_currentTime)) {
      for (auto toNotify : m_dependentInvariantData.at(id)) {
        // If we do multiple "probes" within the same timestamp then the
        // invariant may already have been notified.
        // Also, do not notify invariants that are not active.
        if (m_currentTime == toNotify.lastNotification ||
            !m_propGraph.isActive(m_currentTime, toNotify.id)) {
          continue;
        }
        m_store.getInvariant(toNotify.id)
            .notifyIntChanged(m_currentTime, *this, toNotify.localId,
                              variable.getCommittedValue(),
                              variable.getValue(m_currentTime), toNotify.data);
        toNotify.lastNotification = m_currentTime;
      }
    }
    id = m_propGraph.getNextStableVariable(m_currentTime);
  }
}

/*
// TODO: all of these datastructures should be members.
// TODO: the lambda functions should be inlined private functions!
void Engine::bottomUpPropagate() {
  // TODO: prefill stack with query variables.
  std::vector<VarId> variableStack;
  std::vector<InvariantId> invariantStack;
  std::vector<bool> isUpToDate;
  std::vector<bool> hasVisited;

  auto setVisisted = [&](VarId v) { hasVisited.at(v) = true; };
  auto fixpoint = [&](VarId v) { isUpToDate.at(v) = true; };
  // We expand an invariant by pushing it and its first input variable onto each
  // stack.
  auto expandInvariant = [&](InvariantId inv) {
    VarId nextVar = m_store.getInvariant(inv).getNextDependency(m_currentTime);
    assert(nextVar.id !=
           NULL_ID);  // Invariant must have at least one dependency, and this
                      // should be the first (and only) time we expand it
    variableStack.push_back(nextVar);
    invariantStack.push_back(inv);
  };

  auto notifyCurrentInvariant = [&](VarId id) {
    IntVar variable = m_store.getIntVar(id);
    m_store.getInvariant(invariantStack.back())
        .notifyCurrentDependencyChanged(m_currentTime,
                                        variable.getCommittedValue(),
                                        variable.getValue(m_currentTime));
  };

  auto nextVar = [&]() {
    variableStack.pop_back();
    VarId nextVar = m_store.getInvariant(invariantStack.back())
                        .getNextDependency(m_currentTime);
    if (nextVar.id == NULL_ID) {
      // The invariant has finished propagating, so all defined vars can be
      // marked as up to date.
      // Do this with member array of timestamps.
      for (auto defVar : m_propGraph.variableDefinedBy(invariantStack.back())) {
        fixpoint(defVar);
      }
      invariantStack.pop_back();
    } else {
      variableStack.push_back(nextVar);
    }
  };

  // recursively expand variables to compute their value.
  while (!variableStack.empty()) {
    VarId currentVar = variableStack.back();
    if (hasVisited.at(currentVar)) {
      //TODO: dynamic cycle! throw exception: illegal move or illegal initial assignment.
    }
    setVisisted(currentVar);
    // If the variable has not been expanded before, then expand it.
    if (!isUpToDate.at(currentVar)) {
      if (m_propGraph.m_definingInvariant.at(currentVar).id != NULL_ID) {
        // Variable is defined, so expand defining invariant.
        expandInvariant(m_propGraph.m_definingInvariant.at(currentVar));
        continue;
      } else if (hasChanged(m_currentTime, currentVar)) {
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
      if (hasChanged(m_currentTime, currentVar)) {
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
  // All output variables are up to date!
}

*/

//---------------------Registration---------------------

VarId Engine::makeIntVar(Int initValue) {
  if (!m_isOpen) {
    throw ModelNotOpenException("Cannot make IntVar when store is closed.");
  }
  VarId newId = m_store.createIntVar(initValue);
  m_propGraph.registerVar(newId);
  assert(newId.id == m_dependentInvariantData.size());
  m_dependentInvariantData.push_back({});
  return newId;
}

void Engine::registerInvariantDependsOnVar(InvariantId dependent, VarId source,
                                           LocalId localId, Int data) {
  m_propGraph.registerInvariantDependsOnVar(dependent, source);
  m_dependentInvariantData.at(source).emplace_back(
      InvariantDependencyData{dependent, localId, data, NULL_TIMESTAMP});
}

void Engine::registerDefinedVariable(VarId dependent, InvariantId source) {
  m_propGraph.registerDefinedVariable(dependent, source);
}