#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <deque>
#include <random>
#include <vector>

#include "invariantgraph/fzn/fzn_count_neq.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class fzn_count_neqTest : public ::testing::Test {
 public:
  std::vector<VarNodeId> inputs{};
  Int numInputs = 3;

  void SetUp() override {}
};

}  // namespace atlantis::testing