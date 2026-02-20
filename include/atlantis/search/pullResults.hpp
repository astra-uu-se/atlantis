#pragma once

namespace atlantis::search {

class PullResults {
public:
  const size_t improvingSolutions;

  explicit PullResults(const size_t improvingSolutions):
    improvingSolutions(improvingSolutions) {}

  static std::unique_ptr<PullResults> newResults(const std::unique_ptr<PullResults>& previous, const size_t improvingSolutions) {
    return std::make_unique<PullResults>(improvingSolutions - previous->improvingSolutions);
  }
};

}  // namespace atlantis::search
