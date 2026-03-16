#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include "Parameters.hpp"
#include "LutNode.hpp"
#include "PhenotypeCache.hpp"

namespace evo_circuit {

/// Representação mínima de um nó CGP para o simulador.
/// Em produção será integrada com o Chromosome do cgp-plusplus.
struct CGPNode {
    std::vector<uint32_t> inputs;  ///< índices dos sinais de entrada do nó
    uint32_t function_id = 0;      ///< ID de função (0..127 primitivo, >=128 LUT-k)
};

/// Representação mínima de cromossomo para Fase 1.
struct Chromosome {
    std::vector<CGPNode>  nodes;          ///< todos os nós do grid (ativos e inativos)
    std::vector<uint32_t> output_indices; ///< índices das saídas primárias
    std::vector<LutNode>  lut_tables;     ///< tabelas verdade dos nós LUT-k (paralelo a nodes)
    uint32_t n_inputs = 0;
};

/// Simulador bit-paralelo. Avalia N_BATCH = 64 vetores de entrada simultaneamente.
/// Fase 1: uint64_t escalar (sem xsimd). Fase 6A: substituir por xsimd::batch<uint64_t>.
class BitSimulator {
public:
    explicit BitSimulator(const Parameters& params);

    /// Constrói o PhenotypeCache a partir do genótipo completo (avaliação não-incremental).
    void build_cache(const Chromosome& chrom, PhenotypeCache& cache) const;

    /// Avalia o circuito para um batch de 64 vetores de entrada.
    /// inputs[i]  = uint64_t com o bit j representando o valor da entrada i no vetor j.
    /// outputs[i] = uint64_t resultado da saída i para os 64 vetores.
    /// Usa avaliação incremental se cache.initialized == true.
    void evaluate(const Chromosome& chrom,
                  PhenotypeCache&   cache,
                  const uint64_t*   inputs,   ///< [n_inputs]
                  uint64_t*         outputs,  ///< [n_outputs]
                  bool              incremental = false) const;

private:
    const Parameters& params_;

    /// Percorre o grafo e retorna nós ativos em ordem topológica.
    std::vector<uint32_t> compute_active_nodes(const Chromosome& chrom) const;

    /// Aplica a função primitiva ao nó.
    uint64_t apply_primitive(uint32_t function_id,
                             const std::vector<uint32_t>& input_ids,
                             const uint64_t* node_values) const;

    /// Avalia um nó LUT-k via lookup na tabela verdade.
    uint64_t apply_lut(const LutNode& lut,
                       const std::vector<uint32_t>& input_ids,
                       const uint64_t* node_values) const;
};

} // namespace evo_circuit
