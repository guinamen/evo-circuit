// =============================================================================
// test_phase2.cpp — Testes e evolução real da Fase 2
//
// Benchmark: even-parity 4-bit
//   4 entradas, 16 padrões (2^4) codificados em bitmasks uint32
//   Function set: {AND, OR, NAND, NOR}  — XOR(a,b) requer ~4 nós
//   Critério: fitness=0 (todos os 16 padrões corretos)
//
// Critério de aceitação (Fase 2):
//   ≥ 4/5 runs convergem em ≤ 1.000.000 avaliações
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
    const std::vector<std::string> bases = {
        "", "../", "../../"
    };
    for (const auto& b : bases) {
        std::string p = b + "data/plufiles/" + name;
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

TEST_CASE("BooleanEvolver constroi sem excecao com epar4.plu", "[fase2][init]")
{
    EvolverConfig cfg;
    cfg.verbose = false;
    REQUIRE_NOTHROW([&]() {
        BooleanEvolver evolver(find_plu("epar4.plu"), cfg);
    }());
}

// ── Smoke test ────────────────────────────────────────────────────────────

TEST_CASE("BooleanEvolver evolui even-parity 4-bit (smoke test)", "[fase2][evolution]")
{
    EvolverConfig cfg;
    cfg.n_nodes       = 50;
    cfg.lambda        = 4;
    cfg.mutation_rate = 0.05f;
    cfg.max_evals     = 1'000'000LL;
    cfg.seed          = 1LL;
    cfg.verbose       = false;

    BooleanEvolver evolver(find_plu("epar4.plu"), cfg);
    EvolveResult r = evolver.run();

    INFO("Avaliações: " << r.evals
         << " | Fitness: " << r.fitness
         << " | Wall: "    << r.wall_ms << " ms");

    // Smoke: deve completar sem exceção com valores válidos
    REQUIRE(r.fitness >= 0);
    REQUIRE(r.evals   >  0);
    REQUIRE(r.evals   <= cfg.max_evals);

    if (r.success)
        std::cout << "[SUCESSO] even-parity 4-bit em "
                  << r.evals << " avaliações ("
                  << r.wall_ms << " ms)\n";
    else
        std::cout << "[INFO] Não convergiu (seed=" << cfg.seed
                  << "). Fitness residual: " << r.fitness
                  << " de 16 bits\n";
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
        BooleanEvolver evolver(find_plu("epar4.plu"), cfg);
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

    // Critério Fase 2: ≥ 80% de sucesso
    REQUIRE(successes >= static_cast<int>(N_RUNS * 0.8));
}
