#include "core/engine.hpp"

#include "exceptions/exceptions.hpp"

extern Id NULL_ID;

Engine::Engine()
    : m_currentTime(NULL_TIMESTAMP + 1),
      m_isOpen(false),
      m_store(ESTIMATED_NUM_OBJECTS, NULL_ID) {
  m_intVarViewSource.reserve(ESTIMATED_NUM_OBJECTS);
  m_intVarViewSource.push_back(NULL_ID);

  m_dependantIntVarViews.reserve(ESTIMATED_NUM_OBJECTS);
  m_dependantIntVarViews.push_back({});

  m_dependentInvariantData.reserve(ESTIMATED_NUM_OBJECTS);
  m_dependentInvariantData.push_back({});
}

void Engine::recomputeUsingParent(IntVarView& view, IntVar& var) {
  VarId parentId = view.getParentId();
  Timestamp t;
  if (parentId.idType == VarIdType::var) {
    t = var.getTmpTimestamp();
    view.recompute(t, var.getValue(t), var.getCommittedValue());
    return;
  }
  IntVarView& parent = m_store.getIntVarView(parentId);
  t = parent.getTmpTimestamp();
  view.recompute(t, parent.getValue(t), parent.getCommittedValue());
}

Int Engine::getValue(Timestamp t, VarId v) {
  if (v.idType == VarIdType::var) {
    return m_store.getIntVar(v).getValue(t);
  }
  auto& intVarView = m_store.getIntVarView(v);
  if (intVarView.getTmpTimestamp() == t) {
    return intVarView.getValue(t);
  }
  VarId sourceVarId = m_intVarViewSource.at(v);
  IntVar& sourceVar = m_store.getIntVar(sourceVarId);
  if (sourceVar.getTmpTimestamp() != t) {
    return intVarView.getValue(t);
  }
  if (intVarView.getParentId().idType == VarIdType::var) {
    recomputeUsingParent(intVarView, sourceVar);
    return intVarView.getValue(t);
  }
  auto queue = std::make_unique<std::queue<IntVarView*>>();
  queue->push(&intVarView);
  
  Int prevVal = sourceVar.getValue(t);

  while (queue->back()->getParentId().idType == VarIdType::view) {
    IntVarView& current = m_store.getIntVarView(queue->back()->getParentId());
    // Quick release if current's value is as recent as
    // the source VarId's value
    if (current.getTmpTimestamp() == t) {
      prevVal = current.getValue(t);
      break;
    }
    queue->push(&current);
  }
  
  while (!queue->empty()) {
    queue->front()->recompute(t, prevVal);
    prevVal = queue->front()->getValue(t);
    queue->pop();
  }

  return prevVal;
}

//---------------------Registration---------------------

VarId Engine::makeIntVar(Int initValue, Int lowerBound, Int upperBound) {
  if (!m_isOpen) {
    throw ModelNotOpenException("Cannot make IntVar when store is closed.");
  }
  VarId newId =
      m_store.createIntVar(m_currentTime, initValue, lowerBound, upperBound);
  registerVar(newId);
  assert(newId.id == m_dependentInvariantData.size());
  m_dependentInvariantData.push_back({});
  return newId;
}