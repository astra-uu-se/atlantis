#include <gtest/gtest.h>

#include <stdexcept>

#include "atlantis/search/threadController.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

TEST(ThreadControllerTest, RethrowsRecordedFatalError) {
  ThreadController controller(1);

  controller.recordFatalError(
      std::make_exception_ptr(std::runtime_error("boom")), 7, "solver thread");

  ASSERT_TRUE(controller.hasFatalError());
  try {
    controller.rethrowFatalErrorIfAny();
    FAIL() << "Expected a fatal error to be rethrown";
  } catch (const std::runtime_error& e) {
    EXPECT_NE(std::string(e.what()).find("boom"), std::string::npos);
    EXPECT_NE(std::string(e.what()).find("7"), std::string::npos);
  }
}

TEST(ThreadControllerTest, KeepsFirstFatalError) {
  ThreadController controller(1);

  controller.recordFatalError(
      std::make_exception_ptr(std::runtime_error("first")), 1, "first");
  controller.recordFatalError(
      std::make_exception_ptr(std::runtime_error("second")), 2, "second");

  try {
    controller.rethrowFatalErrorIfAny();
    FAIL() << "Expected a fatal error to be rethrown";
  } catch (const std::runtime_error& e) {
    EXPECT_NE(std::string(e.what()).find("first"), std::string::npos);
    EXPECT_EQ(std::string(e.what()).find("second"), std::string::npos);
  }
}

}  // namespace atlantis::testing
