#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include "atlantis/invariantgraph/fzn/bool_lt.hpp"
#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class bool_ltTest : public ::testing::Test {
 public:
  std::vector<VarNodeId> inputVarNodeIds{};
  Int numInputs = 3;

  void SetUp() override {}
};

}  // namespace atlantis::testing
