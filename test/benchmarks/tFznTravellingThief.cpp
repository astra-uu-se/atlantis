#include <string>

#include "modelTest.hpp"

namespace atlantis::testing {

TEST(FznTravellingThief, Solve) {
  testModelFile("n10_k3_c5000_l10000_u10100_r46.fzn",
                logging::Level::LVL_TRACE);
}

}  // namespace atlantis::testing