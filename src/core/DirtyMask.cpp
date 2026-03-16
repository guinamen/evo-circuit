#include "evo-circuit/core/DirtyMask.hpp"
#include <queue>

namespace evo_circuit {

void DirtyMask::propagate_forward(
        uint32_t node_id,
        const std::vector<std::vector<uint32_t>>& adjacency)
{
    std::queue<uint32_t> q;
    mark_dirty(node_id);
    q.push(node_id);

    while (!q.empty()) {
        uint32_t cur = q.front(); q.pop();
        if (cur >= adjacency.size()) continue;
        for (uint32_t child : adjacency[cur]) {
            if (!is_dirty(child)) {
                mark_dirty(child);
                q.push(child);
            }
        }
    }
}

} // namespace evo_circuit
