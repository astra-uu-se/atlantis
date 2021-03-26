#include "../constraint.hpp"

class ArrayConstraint : public SimpleConstraint {
 public:
  ArrayConstraint(ConstraintBox constraintBox)
      : SimpleConstraint(constraintBox) {}
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
};
