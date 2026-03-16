#pragma once

#include <vector>
#include "atlantis/propagation/types.hpp"

namespace atlantis::propagation {
inline std::vector<VarId> toVarIds(std::vector<VarViewId>&& ids) {
    std::vector<VarId> varIds;
    varIds.reserve(ids.size());
    for (const auto& id : ids) {
        assert(id.isVar());
        varIds.emplace_back(static_cast<VarId>(id));
    }
    return varIds;
}
}