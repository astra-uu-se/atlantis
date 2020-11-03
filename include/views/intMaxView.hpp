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
  IntMaxView(const VarId t_parentId, Int t_max)
      : m_max(t_max), IntVarView(t_parentId) {}

   void init(Engine& e) override;
   
   bool hasChanged(Timestamp t) override;
   Int getValue(Timestamp t) override;
   Int getCommittedValue() override;
   Timestamp getTmpTimestamp() override;
};