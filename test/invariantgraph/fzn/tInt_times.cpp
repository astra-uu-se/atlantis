#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include "atlantis/invariantgraph/fzn/int_times.hpp"
#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class int_timesTest : public ::testing::Test {
 public:
  std::vector<VarNodeId> inputVarNodeIds{};
  Int numInputs = 3;

  void SetUp() override {}
};

}  // namespace atlantis::testing
