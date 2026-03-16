// =============================================================================
// test_phase2.cpp — Testes da Fase 2
//
// Benchmarks progressivos:
//   Nível 1: and2.plu — AND(a,b), trivial, converge em <10 evals
//   Nível 2: xor2.plu — XOR(a,b), requer 1 nó com XOR no FunctionSet
//   Nível 3: add1c.plu — Full adder 1-bit (benchmarking, não critério hard)
//
// Critério de aceitação da Fase 2: ≥ 8/10 runs convergem em and2 e xor2.
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
    for (const auto& b : {"", "../", "../../"}) {
        std::string p = std::string(b) + "data/plufiles/" + name;
        if (fs::exists(p)) return p;
    }
    return "data/plufiles/" + name;
}

// ── Testes unitários ──────────────────────────────────────────────────────

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

// ── Construção sem exceção ────────────────────────────────────────────────

TEST_CASE("BooleanEvolver constroi sem excecao com and2.plu", "[fase2][init]")
{
    EvolverConfig cfg;
    cfg.verbose = false;
    REQUIRE_NOTHROW([&]() { BooleanEvolver e(find_plu("and2.plu"), cfg); }());
}

// ── Nível 1: AND(a,b) — critério hard ────────────────────────────────────

TEST_CASE("BooleanEvolver evolui AND(a,b) com taxa >= 80% em 10 runs", "[fase2][stats]")
{
    EvolverConfig cfg;
    cfg.n_nodes       = 5;
    cfg.lambda        = 4;
    cfg.mutation_rate = 0.05f;
    cfg.max_evals     = 50'000LL;
    cfg.verbose       = false;

    const int N = 10;
    int ok = 0;
    for (int s = 1; s <= N; ++s) {
        cfg.seed = s;
        EvolveResult r = BooleanEvolver(find_plu("and2.plu"), cfg).run();
        std::cout << "  AND seed=" << s << " e=" << r.evals
                  << " f=" << r.fitness << (r.success?" ✓":"") << "\n";
        if (r.success) ++ok;
    }
    std::cout << "AND taxa: " << ok << "/" << N << "\n";
    REQUIRE(ok >= static_cast<int>(N * 0.8));
}

// ── Nível 2: XOR(a,b) — critério hard ────────────────────────────────────

TEST_CASE("BooleanEvolver evolui XOR(a,b) com taxa >= 80% em 10 runs", "[fase2][stats]")
{
    EvolverConfig cfg;
    cfg.n_nodes       = 5;
    cfg.lambda        = 4;
    cfg.mutation_rate = 0.05f;
    cfg.max_evals     = 50'000LL;
    cfg.verbose       = false;

    const int N = 10;
    int ok = 0;
    for (int s = 1; s <= N; ++s) {
        cfg.seed = s;
        EvolveResult r = BooleanEvolver(find_plu("xor2.plu"), cfg).run();
        std::cout << "  XOR seed=" << s << " e=" << r.evals
                  << " f=" << r.fitness << (r.success?" ✓":"") << "\n";
        if (r.success) ++ok;
    }
    std::cout << "XOR taxa: " << ok << "/" << N << "\n";
    REQUIRE(ok >= static_cast<int>(N * 0.8));
}

// ── Nível 3: add1c — smoke test (não é critério hard) ────────────────────

TEST_CASE("BooleanEvolver executa add1c sem excecao (smoke)", "[fase2][smoke]")
{
    EvolverConfig cfg;
    cfg.n_nodes   = 30;
    cfg.lambda    = 4;
    cfg.max_evals = 100'000LL;
    cfg.seed      = 1;
    cfg.verbose   = false;

    EvolveResult r = BooleanEvolver(find_plu("add1c.plu"), cfg).run();
    INFO("fitness=" << r.fitness << " evals=" << r.evals);
    REQUIRE(r.fitness >= 0);
    REQUIRE(r.evals   >  0);
}
