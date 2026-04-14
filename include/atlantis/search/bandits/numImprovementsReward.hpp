#pragma once
#include <memory>
#include "pullResults.hpp"
#include "reward.hpp"

namespace atlantis::search {

class NumImprovementsReward : public Reward {
  double value;

  static const NumImprovementsReward& downcast(const Reward& other) {
    const auto* ptr = dynamic_cast<const NumImprovementsReward*>(&other);
    if (!ptr) throw std::invalid_argument("Reward type mismatch");
    return *ptr;
  }


public:
  explicit NumImprovementsReward(const PullResults& results) : value(results.improvingSolutions) {}

  explicit NumImprovementsReward(const double value) : value(value) {}

 [[nodiscard]] bool operator==(const std::shared_ptr<Reward>& other) const override {
    return value == downcast(*other).value;
  }

  [[nodiscard]] bool operator<(const std::shared_ptr<Reward>& other) const override{
    return value < downcast(*other).value;
  }

  [[nodiscard]] bool operator<=(const std::shared_ptr<Reward>& other) const override{
    return value <= downcast(*other).value;
  }

  [[nodiscard]] std::optional<double> getDouble() const override {
    return value;
  }
  [[nodiscard]] std::optional<double> getDoubleZeroOne() const override {
    return std::nullopt;
  }
  [[nodiscard]] std::optional<Int> getInt() const override {
    return std::nullopt;
  }
  [[nodiscard]] std::optional<Int> getIntZeroOne() const override {
    return std::nullopt;
  }

  [[nodiscard]] static std::shared_ptr<NumImprovementsReward> average(
      const std::optional<std::vector<std::shared_ptr<Reward>>>& rewards) {
    if (!rewards.has_value() || rewards.value().empty()) {
      return std::make_shared<NumImprovementsReward>(0.0);
    }

    double sum = 0;
    for (size_t i = 0; i < rewards.value().size(); i++) {
      sum += rewards.value()[i]->getDouble().value();
    }

    return std::make_shared<NumImprovementsReward>(sum / rewards.value().size());
  }

  void updateAverage(const std::shared_ptr<Reward>& newReward, const size_t newNumRewards) override {
    const auto newValue = downcast(*newReward).value;

    const double newAvg = (value * (newNumRewards - 1) + newValue) / newNumRewards;

    value = newAvg;
  }

  [[nodiscard]] std::string toString() const override {
    return std::to_string(value);
  }
};

}  // namespace atlantis::search
