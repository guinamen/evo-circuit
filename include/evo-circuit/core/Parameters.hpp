#pragma once
#include <cstdint>
#include <vector>

namespace evo_circuit {

/// Parâmetros globais do framework. Passados por referência const em todo o sistema.
struct Parameters {
    // ── Grid CGP ────────────────────────────────────────────────────────
    uint32_t n_inputs   = 2;
    uint32_t n_outputs  = 1;
    uint32_t grid_rows  = 5;
    uint32_t grid_cols  = 10;
    uint32_t k_max      = 2;   ///< aridade máxima dos nós (conexões por nó)
    uint32_t levels_back = 0;  ///< 0 = sem restrição (acesso a todos os nós anteriores)

    // ── Estratégia evolutiva ─────────────────────────────────────────────
    uint32_t lambda         = 4;
    double   mutation_rate  = 0.05;
    double   active_bias    = 0.7;  ///< [0.0, 1.0] — fração da mutação em nós ativos

    // ── LUT-k (Fase A) ───────────────────────────────────────────────────
    double   lut_ratio   = 0.0;     ///< 0.0 = CGP clássico sem LUTs
    std::vector<uint8_t> lut_arities = {2, 3, 4, 5, 6};
    double   lut_bit_rate   = 0.0;  ///< taxa de flip_lut_bit por geração
    double   lut_regen_rate = 0.0;  ///< taxa de randomize_lut por geração

    // ── Fitness progressivo ──────────────────────────────────────────────
    uint32_t max_generations = 1'000'000;

    // ── JIT ──────────────────────────────────────────────────────────────
    bool     jit_enabled   = false;
    uint32_t jit_threshold = 500;   ///< gerações sem melhora antes de compilar elite

    // ── Utilitários ──────────────────────────────────────────────────────
    uint32_t n_nodes() const { return grid_rows * grid_cols; }
};

} // namespace evo_circuit
