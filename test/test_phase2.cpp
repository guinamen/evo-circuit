// =============================================================================
// test_phase2.cpp — Testes e evolução real da Fase 2
//
// Benchmark: full adder 1-bit (add1c.plu)
//   3 entradas (a, b, cin), 2 saídas (sum, carry)
//   sum   = XOR(a, b, cin)  → implementável com ~8 nós NAND
//   carry = MAJ(a, b, cin)  → implementável com ~3 nós AND/OR
//   Function set: {AND=0, OR=1, NAND=2, NOR=3}
//   Gradiente real: AND(a,b) já reduz fitness de 8 para 6
//
// Critério de aceitação (Fase 2, doc seção 14):
//   ES (1+4) converge em ≤ 1.000.000 avaliações em ≥ 4/5 runs
// =============================================================================

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "evo-circuit/fitness/BooleanFitness.hpp"
#include "evo-circuit/evolution/BooleanEvolver.hpp"
#include <filesystem>
#include <string>
#include <vector>
#include <iostream>

using namespace evo_circuit;
namespace fs = std::filesystem;

static std::string find_plu(const std::string& name) {
    for (const auto& base : {"", "../", "../../"}) {
        std::string p = std::string(base) + "data/plufiles/" + name;
        if (fs::exists(p)) return p;
    }
    return "data/plufiles/" + name;
}

// ── Testes unitários de BooleanFitness ───────────────────────────────────

TEST_CASE("BooleanFitness calcula distancia de Hamming corretamente", "[fase2][fitness]")
{
    std::vector<unsigned long> expected = { 0b1001UL };
    std::vector<unsigned long> actual   = { 0b1011UL };
    REQUIRE(BooleanFitness::hamming_distance(actual, expected, 4) == 1);
}

TEST_CASE("BooleanFitness retorna 0 para saidas identicas", "[fase2][fitness]")
{
    std::vector<unsigned long> v = { 0b10110101UL };
    REQUIRE(BooleanFitness::hamming_distance(v, v, 8) == 0);
}

TEST_CASE("BooleanFitness normalizado entre 0 e 1", "[fase2][fitness]")
{
    REQUIRE(BooleanFitness::normalized(3, 8) == Catch::Approx(0.625).epsilon(0.001));
}

// ── Teste de construção ───────────────────────────────────────────────────

TEST_CASE("BooleanEvolver constroi sem excecao com add1c.plu", "[fase2][init]")
{
    EvolverConfig cfg;
    cfg.verbose = false;
    REQUIRE_NOTHROW([&]() {
        BooleanEvolver evolver(find_plu("add1c.plu"), cfg);
    }());
}

// ── Smoke test ────────────────────────────────────────────────────────────

TEST_CASE("BooleanEvolver evolui full-adder 1-bit (smoke test)", "[fase2][evolution]")
{
    EvolverConfig cfg;
    cfg.n_nodes       = 50;
    cfg.lambda        = 4;
    cfg.mutation_rate = 0.05f;
    cfg.max_evals     = 1'000'000LL;
    cfg.seed          = 1LL;
    cfg.verbose       = false;

    BooleanEvolver evolver(find_plu("add1c.plu"), cfg);
    EvolveResult r = evolver.run();

    INFO("Avaliações: " << r.evals
         << " | Fitness: " << r.fitness
         << " | Wall: "    << r.wall_ms << " ms");

    REQUIRE(r.fitness >= 0);
    REQUIRE(r.evals   >  0);
    REQUIRE(r.evals   <= cfg.max_evals);

    if (r.success)
        std::cout << "[SUCESSO] full-adder 1-bit em "
                  << r.evals << " avaliações ("
                  << r.wall_ms << " ms)\n";
    else
        std::cout << "[INFO] Não convergiu (seed=" << cfg.seed
                  << "). Fitness residual: " << r.fitness
                  << " de 16 bits (2 saídas × 8 padrões)\n";
}

// ── Teste estatístico ─────────────────────────────────────────────────────

TEST_CASE("BooleanEvolver: taxa de sucesso >= 80% em 5 runs", "[fase2][stats]")
{
    EvolverConfig cfg;
    cfg.n_nodes       = 50;
    cfg.lambda        = 4;
    cfg.mutation_rate = 0.05f;
    cfg.max_evals     = 1'000'000LL;
    cfg.verbose       = false;

    const int N_RUNS = 5;
    int successes = 0;

    for (int run = 0; run < N_RUNS; ++run) {
        cfg.seed = static_cast<long long>(run + 1);
        BooleanEvolver evolver(find_plu("add1c.plu"), cfg);
        EvolveResult r = evolver.run();

        std::cout << "  Run " << (run+1) << "/" << N_RUNS
                  << " — evals=" << r.evals
                  << " fitness=" << r.fitness
                  << (r.success ? " ✓" : " ✗") << "\n";

        if (r.success) ++successes;
    }

    double rate = static_cast<double>(successes) / N_RUNS;
    std::cout << "Taxa de sucesso: " << successes << "/" << N_RUNS
              << " (" << (rate * 100.0) << "%)\n";

    REQUIRE(successes >= static_cast<int>(N_RUNS * 0.8));
}
