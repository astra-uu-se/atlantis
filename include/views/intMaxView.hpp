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
  IntMaxView(const VarId& t_sourceId, Int t_max);
  
  ~IntMaxView() = default;
  
  inline Int getMax() const;

  inline void init(Timestamp t, Engine& e, const Int& sourceVal, const Int& sourceCommittedVal) override;
  inline void recompute(Timestamp t, Engine& e) override;
  inline void recompute(Timestamp t, const Int& sourceVal, const Int& sourceCommittedVal) override;
  inline void recompute(Timestamp t, const Int& sourceVal) override;
  inline void commitValue(const Int& sourceVal) override;
};

inline Int IntMaxView::getMax() const {
  return m_max;
}