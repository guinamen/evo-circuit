#pragma once
#include <vector>
#include <cstdint>
#include "DirtyMask.hpp"

namespace evo_circuit {

/// Mantém o DAG ativo e os valores calculados entre mutações.
/// Evita reconstrução completa do grafo a cada avaliação.
struct PhenotypeCache {
    /// Nós ativos em ordem topológica (apenas os que contribuem para as saídas).
    std::vector<uint32_t> active_nodes;

    /// Grafo de adjacência para frente: adjacency[i] = descendentes diretos de i.
    std::vector<std::vector<uint32_t>> adjacency;

    /// output_cache[node_id] = resultado do último batch avaliado (1 uint64_t por nó).
    /// Indexado por node_id global (inclui nós de entrada).
    std::vector<uint64_t> output_cache;

    /// DirtyMask corrente — sujos = precisam reavaliação.
    DirtyMask dirty;

    /// Indica se o cache foi inicializado a partir de um genótipo completo.
    bool initialized = false;

    explicit PhenotypeCache(uint32_t n_nodes)
        : output_cache(n_nodes, 0)
        , dirty(n_nodes)
    {}

    /// Invalida completamente o cache (ex: warm-start ou reset de cromossomo).
    void invalidate() {
        dirty.set_all_dirty();
        active_nodes.clear();
        adjacency.clear();
        initialized = false;
    }
};

} // namespace evo_circuit
