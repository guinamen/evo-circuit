#include "evo-circuit/core/BitSimulator.hpp"
#include "evo-circuit/core/LutFunctionSet.hpp"
#include <stdexcept>
#include <algorithm>

namespace evo_circuit {

BitSimulator::BitSimulator(const Parameters& params)
    : params_(params)
{}

// ── Active-node walker ────────────────────────────────────────────────────
std::vector<uint32_t> BitSimulator::compute_active_nodes(const Chromosome& chrom) const
{
    const uint32_t n_inputs = chrom.n_inputs;
    const uint32_t n_nodes  = static_cast<uint32_t>(chrom.nodes.size());

    std::vector<bool>     visited(n_inputs + n_nodes, false);
    std::vector<uint32_t> order;

    // Iniciar pelos índices de saída
    std::vector<uint32_t> stack;
    for (uint32_t out_idx : chrom.output_indices) {
        if (out_idx >= n_inputs && !visited[out_idx]) {
            visited[out_idx] = true;
            stack.push_back(out_idx);
        }
    }

    // DFS para identificar nós ativos
    while (!stack.empty()) {
        uint32_t cur = stack.back(); stack.pop_back();
        order.push_back(cur);

        if (cur < n_inputs) continue; // nó de entrada — sem predecessores
        const CGPNode& node = chrom.nodes[cur - n_inputs];
        for (uint32_t inp : node.inputs) {
            if (inp >= n_inputs && !visited[inp]) {
                visited[inp] = true;
                stack.push_back(inp);
            }
        }
    }

    // Ordem topológica: reverter (DFS pós-ordem inversa)
    std::reverse(order.begin(), order.end());
    return order;
}

// ── Construção do PhenotypeCache ──────────────────────────────────────────
void BitSimulator::build_cache(const Chromosome& chrom, PhenotypeCache& cache) const
{
    cache.active_nodes = compute_active_nodes(chrom);
    cache.dirty.set_all_dirty();

    const uint32_t total = chrom.n_inputs + static_cast<uint32_t>(chrom.nodes.size());
    cache.output_cache.assign(total, 0ULL);

    // Construir grafo de adjacência para propagação de DirtyMask
    cache.adjacency.assign(total, {});
    for (uint32_t node_id : cache.active_nodes) {
        if (node_id < chrom.n_inputs) continue;
        const CGPNode& node = chrom.nodes[node_id - chrom.n_inputs];
        for (uint32_t inp : node.inputs) {
            if (inp < total) cache.adjacency[inp].push_back(node_id);
        }
    }

    cache.initialized = true;
}

// ── Primitivos bit-paralelos ──────────────────────────────────────────────
uint64_t BitSimulator::apply_primitive(uint32_t function_id,
                                       const std::vector<uint32_t>& input_ids,
                                       const uint64_t* nv) const
{
    auto a = [&](int i) -> uint64_t { return nv[input_ids[static_cast<size_t>(i)]]; };

    switch (function_id) {
        case FunctionID::AND:  return a(0) & a(1);
        case FunctionID::OR:   return a(0) | a(1);
        case FunctionID::XOR:  return a(0) ^ a(1);
        case FunctionID::NOT:  return ~a(0);
        case FunctionID::NAND: return ~(a(0) & a(1));
        case FunctionID::NOR:  return ~(a(0) | a(1));
        case FunctionID::XNOR: return ~(a(0) ^ a(1));
        case FunctionID::MUX:  return (a(2) & a(0)) | (~a(2) & a(1)); // sel=a(2)
        default:
            throw std::runtime_error("apply_primitive: ID de função desconhecido: "
                                     + std::to_string(function_id));
    }
}

// ── LUT-k bit-paralelo via lookup por shifts ──────────────────────────────
uint64_t BitSimulator::apply_lut(const LutNode& lut,
                                  const std::vector<uint32_t>& input_ids,
                                  const uint64_t* nv) const
{
    // Para cada um dos 64 vetores simultâneos, extrai o índice na tabela
    // construído pelos bits individuais das k entradas, e produz o bit de saída.
    uint64_t result = 0ULL;
    for (int bit = 0; bit < 64; ++bit) {
        uint32_t lut_idx = 0;
        for (uint8_t ki = 0; ki < lut.k; ++ki) {
            uint64_t inp_val = nv[input_ids[ki]];
            if ((inp_val >> bit) & 1ULL) lut_idx |= (1u << ki);
        }
        if (lut.truth_table.test(lut_idx)) result |= (1ULL << bit);
    }
    return result;
}

// ── Avaliação principal ───────────────────────────────────────────────────
void BitSimulator::evaluate(const Chromosome& chrom,
                             PhenotypeCache&   cache,
                             const uint64_t*   inputs,
                             uint64_t*         outputs,
                             bool              incremental) const
{
    if (!cache.initialized) {
        build_cache(chrom, cache);
        incremental = false;
    }

    const uint32_t n_in = chrom.n_inputs;
    uint64_t* nv = cache.output_cache.data();

    // Copiar entradas para o cache
    for (uint32_t i = 0; i < n_in; ++i) nv[i] = inputs[i];

    // Avaliar nós ativos em ordem topológica
    for (uint32_t node_id : cache.active_nodes) {
        if (node_id < n_in) continue;
        if (incremental && !cache.dirty.is_dirty(node_id)) continue;

        const CGPNode& node = chrom.nodes[node_id - n_in];
        uint64_t val;

        if (LutFunctionSet::is_lut(node.function_id)) {
            const LutNode& lut = chrom.lut_tables[node_id - n_in];
            val = apply_lut(lut, node.inputs, nv);
        } else {
            val = apply_primitive(node.function_id, node.inputs, nv);
        }

        nv[node_id] = val;
        cache.dirty.mark_clean(node_id);
    }

    // Coletar saídas
    for (size_t i = 0; i < chrom.output_indices.size(); ++i) {
        outputs[i] = nv[chrom.output_indices[i]];
    }
}

} // namespace evo_circuit
