#pragma once

#include <memory>
#include <vector>

#include "../core/engine.hpp"
#include "../core/intVar.hpp"
#include "../core/intView.hpp"
#include "../core/types.hpp"

class IntMaxView : public IntView {
 private:
  Int m_max;

 public:
  IntMaxView(const VarId t_parentId, Int t_max)
      : IntView(t_parentId), m_max(t_max) {}

  void init(VarId, Engine& e) override;

  Int getValue(Timestamp t) override;
  Int getCommittedValue() override;
  Timestamp getTmpTimestamp() override;
};