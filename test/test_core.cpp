// =============================================================================
// test_core.cpp — Testes de Fase 1
// Critérios de aceitação (doc seção 14, Fase 1):
//   1. BitSimulator avalia truth table de adder 1-bit corretamente
//   2. active-node walker retorna apenas nós ativos
//   3. PhenotypeCache construído corretamente na inicialização
// =============================================================================
#include <catch2/catch_test_macros.hpp>
#include "evo-circuit/core/BitSimulator.hpp"
#include "evo-circuit/core/PhenotypeCache.hpp"
#include "evo-circuit/core/DirtyMask.hpp"
#include "evo-circuit/core/LutNode.hpp"

using namespace evo_circuit;

// ── Helpers ───────────────────────────────────────────────────────────────

/// Constrói um adder 1-bit (half-adder): sum = XOR(a,b), carry = AND(a,b)
/// Entradas: índices 0 (a) e 1 (b)
/// Nós: índice 2 = XOR(0,1), índice 3 = AND(0,1)
/// Saídas: [2 (sum), 3 (carry)]
static Chromosome make_half_adder() {
    Chromosome c;
    c.n_inputs = 2;

    // Nó 0 (global idx 2): XOR dos inputs 0 e 1 → sum
    CGPNode xor_node;
    xor_node.inputs = {0, 1};
    xor_node.function_id = FunctionID::XOR;
    c.nodes.push_back(xor_node);
    c.lut_tables.push_back(LutNode{}); // placeholder (nó primitivo)

    // Nó 1 (global idx 3): AND dos inputs 0 e 1 → carry
    CGPNode and_node;
    and_node.inputs = {0, 1};
    and_node.function_id = FunctionID::AND;
    c.nodes.push_back(and_node);
    c.lut_tables.push_back(LutNode{}); // placeholder

    c.output_indices = {2, 3}; // [sum, carry]
    return c;
}

// ── Teste 1: adder 1-bit ─────────────────────────────────────────────────
TEST_CASE("BitSimulator avalia half-adder 1-bit corretamente", "[core][fase1]")
{
    Parameters params;
    params.n_inputs  = 2;
    params.n_outputs = 2;
    params.grid_rows = 1;
    params.grid_cols = 2;

    BitSimulator sim(params);
    Chromosome   chrom = make_half_adder();
    PhenotypeCache cache(chrom.n_inputs + static_cast<uint32_t>(chrom.nodes.size()));

    // Codificar os 4 vetores de entrada simultaneamente em 64 bits:
    // bit 0: a=0,b=0  bit 1: a=0,b=1  bit 2: a=1,b=0  bit 3: a=1,b=1
    //
    // inputs[0] = a: bits 0=0, 1=0, 2=1, 3=1 → 0b...1100 = 0xC
    // inputs[1] = b: bits 0=0, 1=1, 2=0, 3=1 → 0b...1010 = 0xA
    uint64_t inputs[2]  = { 0xCULL, 0xAULL };
    uint64_t outputs[2] = { 0, 0 };

    sim.evaluate(chrom, cache, inputs, outputs);

    // sum  = XOR(a,b): 0^0=0, 0^1=1, 1^0=1, 1^1=0 → bits 0,1,2,3 = 0110 → 0x6
    // carry= AND(a,b): 0&0=0, 0&1=0, 1&0=0, 1&1=1 → bits 0,1,2,3 = 0001 → 0x8 (bit 3)
    REQUIRE((outputs[0] & 0xFULL) == 0x6ULL); // sum
    REQUIRE((outputs[1] & 0xFULL) == 0x8ULL); // carry
}

// ── Teste 2: active-node walker ───────────────────────────────────────────
TEST_CASE("PhenotypeCache contem apenas nos ativos apos build_cache", "[core][fase1]")
{
    Parameters params;
    params.n_inputs  = 2;
    params.n_outputs = 1;
    params.grid_rows = 1;
    params.grid_cols = 3;

    Chromosome c;
    c.n_inputs = 2;

    // Nó 0 (global 2): XOR(0,1) — ativo (usado na saída)
    CGPNode n0; n0.inputs = {0,1}; n0.function_id = FunctionID::XOR;
    c.nodes.push_back(n0); c.lut_tables.push_back(LutNode{});

    // Nó 1 (global 3): AND(0,1) — INATIVO (não usado)
    CGPNode n1; n1.inputs = {0,1}; n1.function_id = FunctionID::AND;
    c.nodes.push_back(n1); c.lut_tables.push_back(LutNode{});

    // Nó 2 (global 4): OR(0,1) — INATIVO
    CGPNode n2; n2.inputs = {0,1}; n2.function_id = FunctionID::OR;
    c.nodes.push_back(n2); c.lut_tables.push_back(LutNode{});

    c.output_indices = {2}; // somente o XOR

    BitSimulator   sim(params);
    PhenotypeCache cache(c.n_inputs + static_cast<uint32_t>(c.nodes.size()));

    sim.build_cache(c, cache);

    REQUIRE(cache.initialized == true);
    REQUIRE(cache.active_nodes.size() == 1);
    REQUIRE(cache.active_nodes[0] == 2); // somente o nó XOR
}

// ── Teste 3: PhenotypeCache inicializado corretamente ─────────────────────
TEST_CASE("PhenotypeCache inicializado com DirtyMask all-dirty", "[core][fase1]")
{
    Parameters params;
    params.n_inputs  = 2;
    params.n_outputs = 2;

    Chromosome     chrom = make_half_adder();
    PhenotypeCache cache(chrom.n_inputs + static_cast<uint32_t>(chrom.nodes.size()));
    BitSimulator   sim(params);

    REQUIRE(cache.initialized == false);

    sim.build_cache(chrom, cache);

    REQUIRE(cache.initialized == true);
    REQUIRE(cache.output_cache.size() == 4); // 2 inputs + 2 nodes
    REQUIRE(cache.active_nodes.size() == 2); // XOR e AND
}

// ── Teste 4: DirtyMask — propagação forward ───────────────────────────────
TEST_CASE("DirtyMask propaga sujeira para frente no DAG", "[core][fase1]")
{
    // Grafo: 0 → 1 → 2 → 3 (cadeia linear)
    DirtyMask mask(4);
    mask.set_all_dirty(); // garante estado inicial conhecido

    std::vector<std::vector<uint32_t>> adj = {
        {1},    // 0 → 1
        {2},    // 1 → 2
        {3},    // 2 → 3
        {}      // 3 (folha)
    };

    // Limpar tudo e sujar apenas o nó 1
    for (uint32_t i = 0; i < 4; ++i) mask.mark_clean(i);
    mask.propagate_forward(1, adj);

    REQUIRE(mask.is_dirty(0) == false); // ancestral — não afetado
    REQUIRE(mask.is_dirty(1) == true);  // mutado
    REQUIRE(mask.is_dirty(2) == true);  // descendente
    REQUIRE(mask.is_dirty(3) == true);  // descendente
}

// ── Teste 5: LutNode — avaliação correta ─────────────────────────────────
TEST_CASE("LutNode avalia LUT-2 (XOR) corretamente", "[core][fase1]")
{
    LutNode lut;
    lut.k = 2;
    // XOR: tt[0]=0(00), tt[1]=1(01), tt[2]=1(10), tt[3]=0(11)
    lut.truth_table = 0b0110; // bits 0=0, 1=1, 2=1, 3=0

    REQUIRE(lut.evaluate(0b00) == false); // 0 XOR 0 = 0
    REQUIRE(lut.evaluate(0b01) == true);  // 0 XOR 1 = 1
    REQUIRE(lut.evaluate(0b10) == true);  // 1 XOR 0 = 1
    REQUIRE(lut.evaluate(0b11) == false); // 1 XOR 1 = 0
}
