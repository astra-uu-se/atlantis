#pragma once

#include <memory>
#include <vector>

#include "views/intView.hpp"

class IntMaxView : public IntView {
 private:
  Int m_max;

 public:
  IntMaxView(const VarId t_parentId, Int t_max)
      : IntView(t_parentId), m_max(t_max) {}

  Int getValue(Timestamp t) override;
  Int getCommittedValue() override;
  Int getLowerBound() override;
  Int getUpperBound() override;
};