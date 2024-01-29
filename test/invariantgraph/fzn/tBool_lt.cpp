#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <deque>
#include <random>
#include <vector>

#include "invariantgraph/fzn/bool_lt.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class bool_ltTest : public ::testing::Test {
 public:
  std::vector<VarNodeId> inputs{};
  Int numInputs = 3;

  void SetUp() override {}
};

}  // namespace atlantis::testing