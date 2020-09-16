#pragma once
#include <memory>

#include "core/intVar.hpp"
#include "core/savedInt.hpp"
#include "core/tracer.hpp"
#include "core/types.hpp"
#include "core/var.hpp"
#include "core/varView.hpp"

class Engine; // Forward declaration

class IntVarView : public VarView {
 protected:
  SavedInt m_savedInt;
  
  friend class Engine;

 public:
  IntVarView(const VarId& t_parentId);
  
  virtual ~IntVarView() {};
  
  bool hasChanged(Timestamp t) const;
  Int getValue(Timestamp t) const noexcept;
  Int getCommittedValue() const noexcept;
  Timestamp getTmpTimestamp() const noexcept;
  
  virtual void init(Timestamp t, Engine& e, const Int& sourceVal, const Int& sourceCommittedVal) = 0;
  virtual void recompute(Timestamp t, Engine& e) = 0;
  virtual void recompute(Timestamp t, const Int& sourceVal) = 0;
  virtual void recompute(Timestamp t, const Int& sourceVal, const Int& sourceCommittedVal) = 0;
  inline void commit();
  inline void commitIf(Timestamp t);
  virtual void commitValue(const Int& sourceVal) = 0;
};
inline bool IntVarView::hasChanged(Timestamp t) const {
  return m_savedInt.hasChanged(t);
}

inline Int IntVarView::getValue(Timestamp t) const noexcept {
  return m_savedInt.getValue(t);
}

inline Int IntVarView::getCommittedValue() const noexcept {
  return m_savedInt.getCommittedValue();
}

inline Timestamp IntVarView::getTmpTimestamp() const noexcept {
  return m_savedInt.getTmpTimestamp();
}

inline void IntVarView::commit() {
  return m_savedInt.commit();
}

inline void IntVarView::commitIf(Timestamp t) {
  return m_savedInt.commitIf(t);
}