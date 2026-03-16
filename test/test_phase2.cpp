// =============================================================================
// test_phase2.cpp — Testes da Fase 2
//
// Parâmetros calibrados por diagnóstico empírico:
//   and2: n=10 lb=10 λ=4 mut=0.05 max_evals=50k → converge em <10 evals (seeds favoráveis)
//   xor2: n=10 lb=10 λ=4 mut=0.05 max_evals=50k → converge com XOR disponível
//
// Nota sobre o platô de fitness=2 (and2) e fitness=4 (xor2):
//   Com entradas bitmask {240,204}, a saída constante 0 tem fitness=2 para AND
//   e fitness=4 para XOR. Algumas seeds ficam nesse mínimo local.
//   Com 10 runs e 50k evals, ≥80% convergem empiricamente.
// =============================================================================

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "evo-circuit/fitness/BooleanFitness.hpp"
#include "evo-circuit/evolution/BooleanEvolver.hpp"
#include <filesystem>
#include <string>
#include <iostream>

using namespace evo_circuit;
namespace fs = std::filesystem;

static std::string find_plu(const std::string& name) {
    for (const auto& b : {"", "../", "../../"}) {
        std::string p = std::string(b) + "data/plufiles/" + name;
        if (fs::exists(p)) return p;
    }
    return "data/plufiles/" + name;
}

// ── Unitários ────────────────────────────────────────────────────────────

TEST_CASE("BooleanFitness calcula distancia de Hamming corretamente", "[fase2][fitness]")
{
    std::vector<unsigned long> exp = {0b1001UL}, got = {0b1011UL};
    REQUIRE(BooleanFitness::hamming_distance(got, exp, 4) == 1);
}

TEST_CASE("BooleanFitness retorna 0 para saidas identicas", "[fase2][fitness]")
{
    std::vector<unsigned long> v = {0b10110101UL};
    REQUIRE(BooleanFitness::hamming_distance(v, v, 8) == 0);
}

TEST_CASE("BooleanFitness normalizado entre 0 e 1", "[fase2][fitness]")
{
    REQUIRE(BooleanFitness::normalized(3, 8) == Catch::Approx(0.625).epsilon(0.001));
}

// ── Construção ────────────────────────────────────────────────────────────

TEST_CASE("BooleanEvolver constroi sem excecao", "[fase2][init]")
{
    EvolverConfig cfg;
    cfg.verbose = false;
    REQUIRE_NOTHROW([&](){ BooleanEvolver(find_plu("and2.plu"), cfg); }());
}

// ── AND(a,b) — critério hard ──────────────────────────────────────────────

TEST_CASE("BooleanEvolver evolui AND com taxa >= 80% em 10 runs", "[fase2][stats]")
{
    EvolverConfig cfg;
    cfg.n_nodes       = 10;
    cfg.lambda        = 4;
    cfg.levels_back   = 10;
    cfg.mutation_rate = 0.05f;
    cfg.max_evals     = 50'000LL;
    cfg.verbose       = false;

    const int N = 10;
    int ok = 0;
    for (int s = 1; s <= N; ++s) {
        cfg.seed = s;
        auto r = BooleanEvolver(find_plu("and2.plu"), cfg).run();
        std::cout << "  AND seed=" << s << " evals=" << r.evals
                  << " fitness=" << r.fitness << (r.success?" ✓":"") << "\n";
        if (r.success) ++ok;
    }
    std::cout << "AND taxa: " << ok << "/" << N
              << " (" << (ok*10) << "%)\n";
    REQUIRE(ok >= static_cast<int>(N * 0.8));
}

// ── XOR(a,b) — critério hard ──────────────────────────────────────────────

TEST_CASE("BooleanEvolver evolui XOR com taxa >= 80% em 10 runs", "[fase2][stats]")
{
    EvolverConfig cfg;
    cfg.n_nodes       = 10;
    cfg.lambda        = 4;
    cfg.levels_back   = 10;
    cfg.mutation_rate = 0.05f;
    cfg.max_evals     = 50'000LL;
    cfg.verbose       = false;

    const int N = 10;
    int ok = 0;
    for (int s = 1; s <= N; ++s) {
        cfg.seed = s;
        auto r = BooleanEvolver(find_plu("xor2.plu"), cfg).run();
        std::cout << "  XOR seed=" << s << " evals=" << r.evals
                  << " fitness=" << r.fitness << (r.success?" ✓":"") << "\n";
        if (r.success) ++ok;
    }
    std::cout << "XOR taxa: " << ok << "/" << N
              << " (" << (ok*10) << "%)\n";
    REQUIRE(ok >= static_cast<int>(N * 0.8));
}

// ── add1c smoke ───────────────────────────────────────────────────────────

TEST_CASE("BooleanEvolver executa add1c sem excecao", "[fase2][smoke]")
{
    EvolverConfig cfg;
    cfg.n_nodes   = 30;
    cfg.lambda    = 4;
    cfg.levels_back = 30;
    cfg.max_evals = 100'000LL;
    cfg.seed      = 1;
    cfg.verbose   = false;
    auto r = BooleanEvolver(find_plu("add1c.plu"), cfg).run();
    INFO("fitness=" << r.fitness << " evals=" << r.evals);
    REQUIRE(r.fitness >= 0);
    REQUIRE(r.evals   >  0);
}
