#pragma once

#include <memory>
#include <vector>

#include "../core/engine.hpp"
#include "../core/intVar.hpp"
#include "../core/intVarView.hpp"
#include "../core/types.hpp"

class IntMaxView : public IntVarView {
 private:
  Int m_max;
  
 public:
  IntMaxView(const VarId& t_parentId, Int t_max);
  
  ~IntMaxView() = default;
  
  inline Int getMax() const;

  void init(Timestamp t, Engine& e, Int parentVal, Int parentCommittedVal) override;
  void recompute(Timestamp t, Int parentVal, Int parentCommittedVal) override;
  void recompute(Timestamp t, Int parentVal) override;
  void commitValue(Int parentVal) override;
};

inline Int IntMaxView::getMax() const {
  return m_max;
}