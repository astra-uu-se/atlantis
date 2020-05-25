// taken from:
// https://github.com/athanor/athanor/blob/master/src/types/intVal.h
#ifndef SRC_DOMAIN_INTDOMAIN_H_
#define SRC_DOMAIN_INTDOMAIN_H_

#include <numeric>
#include <utility>
#include <vector>
#include <memory>
#include <algorithm>
#include <variant>

inline auto intBound(int a, int b) { return std::make_pair(a, b); }
struct IntDomain {
    // The domain is a vector of ranges where each range has a lower and upper bound
    std::vector<std::pair<int, int>> bounds;
    // The number of values in the domain
    unsigned int domainSize;
    
    IntDomain(std::vector<std::pair<int, int>> boundRanges)
        : bounds(normaliseBounds(std::move(boundRanges))),
          domainSize(calculateDomainSize(this->bounds)) {}

    inline std::shared_ptr<IntDomain> deepCopy(std::shared_ptr<IntDomain>&) {
        return std::make_shared<IntDomain>(*this);
    }


    // Retrieves the domain size
    static inline unsigned int calculateDomainSize(const std::vector<std::pair<int, int>>& bounds) {
        // the domain size is sum over all the length of the ranges of the domain
        return std::accumulate(
            bounds.begin(), bounds.end(), 0, [](unsigned int total, auto& range) {
                return total + (range.second - range.first) + 1;
            });
    }

    // sort and unify overlaps
    static std::vector<std::pair<int, int>> normaliseBounds(std::vector<std::pair<int, int>> bounds) {
        if (bounds.size() == 0) {
            return bounds;
        }

        // Sort the bound ranges
        std::sort(bounds.begin(), bounds.end());
        // remove invalid bound ranges
        std::remove_if(
            bounds.begin(),
            bounds.end(),
            [](auto& b) { return b.second < b.first; }
        );
        
        // for each bound range
        for (size_t i = 0; i < bounds.size() - 1; i++) {
            auto& bound = bounds[i];
            auto& nextBound = bounds[i + 1];
            // Do the bound ranges overlap or form a contiguous domain?
            if (bound.second >= nextBound.first - 1) {
                // Merge the bound ranges
                bound.second = std::max(bound.second, nextBound.second);
                bounds.erase(bounds.begin() + i + 1);
            }
        }
        return bounds;
    }

    void merge(const IntDomain& other) {
        bounds.insert(bounds.end(), other.bounds.begin(), other.bounds.end());
        bounds = normaliseBounds(std::move(bounds));
        domainSize = calculateDomainSize(bounds);
    }
    // structs for possible return values of findContainingBound function below
    struct FoundBound {
        size_t index;
        FoundBound(size_t boundIndex) : index(boundIndex) {}
    };

    struct BetweenBounds {
        size_t lower;
        size_t upper;
        BetweenBounds(size_t lowerBoundIndex, size_t upperBoundIndex)
            : lower(lowerBoundIndex), upper(upperBoundIndex) {}
    };
    struct OutOfBounds {};
    struct OutOfBoundsSmall {};
    struct OutOfBoundsLarge {};
    typedef std::variant<FoundBound, BetweenBounds, OutOfBounds,
                         OutOfBoundsSmall, OutOfBoundsLarge>
        ContainingBound;
    
    inline ContainingBound findContainingBound(int value) const {
        if (bounds.size() == 0) {
            return OutOfBounds();
        }
        if (value < bounds.front().first) {
            return OutOfBoundsSmall();
        }
        if (value > bounds.back().second) {
            return OutOfBoundsLarge();
        }
        int left = 0;
        int right = bounds.size() - 1;
        int mid;
        while (left <= right) {
            mid = left + (right - left) / 2;
            const auto& bound = bounds[mid];
            if (bound.first <= value && value <= bound.second) {
                return FoundBound(mid);
            }
            if (value < bound.first) {
                if (mid == left) {
                    return BetweenBounds(mid - 1, mid);
                }
                right = mid - 1;
                continue;
            }
            if (value > bound.second) {
                if (mid == right) {
                    return BetweenBounds(mid, mid + 1);
                }
                left = mid + 1;
                continue;
            }
        }
        return OutOfBounds();
    }

    inline bool containsValue(int value) const {
        auto c = findContainingBound(value);
        return std::get_if<FoundBound>(&c);
    }
};
#endif