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

struct IntDomain {
    // The domain is a vector of ranges where each range has a lower and upper bound
    int lowerBound;
    int upperBound;
    // The number of values in the domain
    unsigned int domainSize;
    
    IntDomain(int lower, int upper)
        : lowerBound(lower),
          upperBound(upper),
          domainSize(upper - lower + 1) {}

    inline std::shared_ptr<IntDomain> deepCopy(std::shared_ptr<IntDomain>&) {
        return std::make_shared<IntDomain>(*this);
    }

    void merge(const IntDomain& other) {
        lowerBound = std::min(lowerBound, other.lowerBound);
        upperBound = std::max(upperBound, other.upperBound);
        domainSize = upperBound - lowerBound + 1;
    }

    inline bool containsValue(int value) const {
        return lowerBound <= value && value <= upperBound;
    }
};
#endif