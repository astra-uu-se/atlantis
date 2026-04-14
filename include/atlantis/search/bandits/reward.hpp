#pragma once
#include <memory>

namespace atlantis::search {

class Reward {
public:

  virtual ~Reward() {}

  // These should always work
  [[nodiscard]] virtual bool operator==(const std::shared_ptr<Reward>& other) const = 0;
  [[nodiscard]] virtual bool operator<(const std::shared_ptr<Reward>& other) const = 0;
  [[nodiscard]] virtual bool operator<=(const std::shared_ptr<Reward>& other) const = 0;

  // This is a family of methods that may work depending on reward design
  [[nodiscard]] virtual std::optional<double> getDouble() const = 0;
  [[nodiscard]] virtual std::optional<double> getDoubleZeroOne() const = 0;
  [[nodiscard]] virtual std::optional<Int> getInt() const = 0;
  [[nodiscard]] virtual std::optional<Int> getIntZeroOne() const = 0;

  virtual void updateAverage(const std::shared_ptr<Reward>& newReward,
                             size_t newNumRewards) = 0;

  [[nodiscard]] virtual std::string toString() const = 0;
};

}  // namespace atlantis::search
