#pragma once

#include <memory>
#include <vector>

#include "views/intView.hpp"

class IntMaxView : public IntView {
 private:
  Int _max;

 public:
  IntMaxView(const VarId parentId, Int max) : IntView(parentId), _max(max) {}

  Int value(Timestamp) override;
  Int committedValue() override;
  Int lowerBound() override;
  Int upperBound() override;
};