#pragma once
#include <vector>
#include <cstdint>

namespace evo_circuit {

/// Bitset sobre node_ids que rastreia quais nós precisam ser reavaliados.
/// Após mutação do nó i, todos os descendentes de i no DAG ativo são marcados sujos.
class DirtyMask {
public:
    explicit DirtyMask(uint32_t n_nodes) : mask_(n_nodes, true) {}

    void mark_dirty(uint32_t node_id)  { mask_.at(node_id) = true; }
    void mark_clean(uint32_t node_id)  { mask_.at(node_id) = false; }
    bool is_dirty(uint32_t node_id) const { return mask_.at(node_id); }

    /// Marca todos os nós como sujos (usado na inicialização ou após reset completo).
    void set_all_dirty() { std::fill(mask_.begin(), mask_.end(), true); }

    /// Propaga sujeira para frente no DAG ativo a partir de node_id.
    /// adjacency[i] = lista de descendentes diretos de i no grafo ativo.
    void propagate_forward(uint32_t node_id,
                           const std::vector<std::vector<uint32_t>>& adjacency);

    uint32_t size() const { return static_cast<uint32_t>(mask_.size()); }

private:
    std::vector<bool> mask_;
};

} // namespace evo_circuit
