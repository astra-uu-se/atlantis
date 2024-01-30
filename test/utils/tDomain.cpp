#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "types.hpp"
#include "utils/domains.hpp"

namespace atlantis::testing {

static bool domainCoversInterval(const std::vector<DomainEntry>& domain,
                                 Int intervalLb, Int intervalUb) {
  return std::any_of(domain.begin(), domain.end(), [&](const DomainEntry& entry) {
    return entry.lowerBound <= intervalLb && intervalUb <= entry.upperBound;
  });
}

static bool intersects(const std::vector<DomainEntry>& domain, Int intervalLb,
                       Int intervalUb) {
  return std::any_of(domain.begin(), domain.end(), [&](const DomainEntry& entry) {
    return (entry.lowerBound <= intervalLb && intervalLb <= entry.upperBound) ||
           (intervalLb <= entry.lowerBound && entry.lowerBound <= intervalUb);
  });
}

class DomainTest : public ::testing::Test {
 protected:
  void SetUp() override {
    std::random_device rd;
    gen = std::mt19937(rd());
  }
  std::mt19937 gen;
};

TEST_F(DomainTest, relativeComplementIfIntersects) {
  std::vector<std::vector<DomainEntry>> domains{
      std::vector<DomainEntry>{{0, 0}},
      std::vector<DomainEntry>{{0, 10}},
      std::vector<DomainEntry>{{0, 20}},
      std::vector<DomainEntry>{{0, 30}},
      std::vector<DomainEntry>{{10, 10}},
      std::vector<DomainEntry>{{10, 20}},
      std::vector<DomainEntry>{{10, 30}},
      std::vector<DomainEntry>{{20, 20}},
      std::vector<DomainEntry>{{20, 30}},
      std::vector<DomainEntry>{{30, 30}},
      std::vector<DomainEntry>{{0, 0}, {10, 10}},
      std::vector<DomainEntry>{{0, 0}, {10, 20}},
      std::vector<DomainEntry>{{0, 0}, {10, 30}},
      std::vector<DomainEntry>{{0, 10}, {20, 20}},
      std::vector<DomainEntry>{{0, 10}, {20, 30}},
      std::vector<DomainEntry>{{0, 20}, {30, 30}},
      std::vector<DomainEntry>{{0, 0}, {10, 10}, {20, 20}},
      std::vector<DomainEntry>{{0, 0}, {10, 10}, {20, 30}},
      std::vector<DomainEntry>{{0, 0}, {10, 10}, {30, 30}},
      std::vector<DomainEntry>{{0, 0}, {10, 20}, {30, 30}},
      std::vector<DomainEntry>{{0, 10}, {20, 20}, {30, 30}},
      std::vector<DomainEntry>{{0, 0}, {10, 10}, {20, 20}, {30, 30}}};

  const Int lb = -10;
  const Int ub = 40;
  const Int inc = 5;

  for (const std::vector<DomainEntry>& dom : domains) {
    std::vector<Int> values;
    for (const DomainEntry& entry : dom) {
      for (Int val = entry.lowerBound; val <= entry.upperBound; ++val) {
        values.push_back(val);
      }
    }
    const Int domLb = *std::min_element(values.begin(), values.end());
    const Int domUb = *std::max_element(values.begin(), values.end());

    for (Int intervalLb = lb; intervalLb <= domUb; intervalLb += inc) {
      for (Int intervalUb = std::max(lb, domLb); intervalUb <= ub;
           intervalUb += inc) {
        SetDomain setDomain(values);
        const std::vector<DomainEntry> setComplement =
            setDomain.relativeComplementIfIntersects(intervalLb, intervalUb);

        if (intersects(dom, intervalLb, intervalUb)) {
          EXPECT_EQ(setComplement.empty(),
                    domainCoversInterval(dom, intervalLb, intervalUb));
        }

        for (const auto complementEntry : setComplement) {
          EXPECT_GE(complementEntry.lowerBound, intervalLb);
          EXPECT_GE(complementEntry.upperBound, intervalLb);

          EXPECT_LE(complementEntry.lowerBound, intervalUb);
          EXPECT_LE(complementEntry.upperBound, intervalUb);
        }

        for (size_t i = 1; i < setComplement.size(); ++i) {
          EXPECT_LT(setComplement.at(i - 1).upperBound,
                    setComplement.at(i).lowerBound);
        }

        if (dom.size() != 1) {
          continue;
        }

        IntervalDomain intervalDomain(dom.front().lowerBound,
                                      dom.back().upperBound);
        const std::vector<DomainEntry> intervalComplement =
            intervalDomain.relativeComplementIfIntersects(intervalLb,
                                                          intervalUb);
        if (intersects(dom, intervalLb, intervalUb)) {
          EXPECT_EQ(intervalComplement.empty(),
                    domainCoversInterval(dom, intervalLb, intervalUb));
        }

        EXPECT_LE(intervalComplement.size(), 1);

        EXPECT_EQ(setComplement.size(), intervalComplement.size());
        for (size_t i = 0; i < setComplement.size(); ++i) {
          EXPECT_EQ(setComplement.at(i).lowerBound,
                    intervalComplement.at(i).lowerBound);
          EXPECT_EQ(setComplement.at(i).upperBound,
                    intervalComplement.at(i).upperBound);
        }

        if (!intervalComplement.empty()) {
          EXPECT_GE(intervalComplement.front().lowerBound, intervalLb);
          EXPECT_GE(intervalComplement.front().upperBound, intervalLb);

          EXPECT_LE(intervalComplement.front().lowerBound, intervalUb);
          EXPECT_LE(intervalComplement.front().upperBound, intervalUb);
        }
      }
    }
  }
}
}  // namespace atlantis::testing