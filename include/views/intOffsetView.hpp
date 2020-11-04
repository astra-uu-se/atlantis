#pragma once

#include <memory>
#include <vector>

#include "../core/engine.hpp"
#include "../core/intVar.hpp"
#include "../core/intView.hpp"
#include "../core/types.hpp"

class IntOffsetView : public IntView {
 private:
  Int m_offset;

 public:
  IntOffsetView(const VarId t_parentId, Int t_offset)
      : IntView(t_parentId), m_offset(t_offset) {}

  Int getValue(Timestamp t) override;
  Int getCommittedValue() override;
};