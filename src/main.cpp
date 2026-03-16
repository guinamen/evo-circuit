// =============================================================================
// main.cpp — evo-circuit CLI
//
// Uso:
//   evo-circuit <arquivo.plu> [opções]
//
// Opções:
//   --nodes N        Número de nós no grid (padrão: 50)
//   --lambda N       Filhos por geração ES 1+λ (padrão: 4)
//   --evals N        Máximo de avaliações de fitness (padrão: 1000000)
//   --mutation F     Taxa de mutação (padrão: 0.05)
//   --seed N         Seed aleatória (padrão: 42; 0=aleatória)
//   --runs N         Número de runs independentes (padrão: 1)
//   --verbose        Exibir progresso a cada 10000 gerações
//   --help           Exibir esta ajuda
//
// Exemplos:
//   ./evo-circuit data/plufiles/add1c.plu
//   ./evo-circuit data/plufiles/add1c.plu --nodes 100 --runs 5
//   ./evo-circuit data/plufiles/add2c.plu --nodes 200 --evals 5000000
// =============================================================================

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <cstring>

#include "evo-circuit/evolution/BooleanEvolver.hpp"

namespace fs = std::filesystem;

static void print_help(const char* prog) {
    std::cout
        << "Uso: " << prog << " <arquivo.plu> [opções]\n\n"
        << "Opções:\n"
        << "  --nodes N      Nós no grid CGP        (padrão: 50)\n"
        << "  --lambda N     Filhos ES 1+λ           (padrão: 4)\n"
        << "  --evals N      Máx. avaliações         (padrão: 1000000)\n"
        << "  --mutation F   Taxa de mutação         (padrão: 0.05)\n"
        << "  --seed N       Seed (0=aleatória)      (padrão: 42)\n"
        << "  --runs N       Runs independentes      (padrão: 1)\n"
        << "  --verbose      Progresso por geração\n"
        << "  --help         Esta ajuda\n\n"
        << "Exemplos:\n"
        << "  " << prog << " data/plufiles/add1c.plu\n"
        << "  " << prog << " data/plufiles/add1c.plu --nodes 100 --runs 5\n"
        << "  " << prog << " data/plufiles/add2c.plu --nodes 200 --evals 5000000\n";
}

static void print_banner(const std::string& plu_file,
                         const evo_circuit::EvolverConfig& cfg,
                         int n_runs) {
    std::cout
        << "\n╔══════════════════════════════════════════════╗\n"
        << "║           evo-circuit v0.1                   ║\n"
        << "╚══════════════════════════════════════════════╝\n"
        << "  Benchmark   : " << fs::path(plu_file).filename().string() << "\n"
        << "  Nós CGP     : " << cfg.n_nodes      << "\n"
        << "  λ (filhos)  : " << cfg.lambda        << "\n"
        << "  Máx. evals  : " << cfg.max_evals     << "\n"
        << "  Mutação     : " << cfg.mutation_rate << "\n"
        << "  Seed base   : " << cfg.seed          << "\n"
        << "  Runs        : " << n_runs            << "\n"
        << "────────────────────────────────────────────────\n";
}

int main(int argc, char* argv[]) {

    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    // ── Parse de argumentos ───────────────────────────────────────────────
    std::string plu_file;
    evo_circuit::EvolverConfig cfg;
    int n_runs = 1;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            print_help(argv[0]);
            return 0;
        } else if (arg == "--verbose" || arg == "-v") {
            cfg.verbose        = true;
            cfg.report_interval = 10000;
        } else if (arg == "--nodes" && i+1 < argc) {
            cfg.n_nodes = std::stoi(argv[++i]);
        } else if (arg == "--lambda" && i+1 < argc) {
            cfg.lambda = std::stoi(argv[++i]);
        } else if (arg == "--evals" && i+1 < argc) {
            cfg.max_evals = std::stoll(argv[++i]);
        } else if (arg == "--mutation" && i+1 < argc) {
            cfg.mutation_rate = std::stof(argv[++i]);
        } else if (arg == "--seed" && i+1 < argc) {
            cfg.seed = std::stoll(argv[++i]);
        } else if (arg == "--runs" && i+1 < argc) {
            n_runs = std::stoi(argv[++i]);
        } else if (arg[0] != '-') {
            plu_file = arg;
        } else {
            std::cerr << "Argumento desconhecido: " << arg << "\n";
            return 1;
        }
    }

    if (plu_file.empty()) {
        std::cerr << "Erro: arquivo .plu não especificado.\n";
        print_help(argv[0]);
        return 1;
    }

    if (!fs::exists(plu_file)) {
        std::cerr << "Erro: arquivo não encontrado: " << plu_file << "\n";
        return 1;
    }

    print_banner(plu_file, cfg, n_runs);

    // ── Executar runs ─────────────────────────────────────────────────────
    int successes = 0;
    long long total_evals = 0;
    double    total_ms    = 0.0;

    auto wall_start = std::chrono::steady_clock::now();

    for (int run = 0; run < n_runs; ++run) {
        long long seed = (cfg.seed == 0) ? 0 : cfg.seed + run;
        evo_circuit::EvolverConfig run_cfg = cfg;
        run_cfg.seed = seed;

        if (n_runs > 1)
            std::cout << "\n  ── Run " << (run+1) << "/" << n_runs
                      << " (seed=" << seed << ") ──\n";

        evo_circuit::BooleanEvolver evolver(plu_file, run_cfg);
        evo_circuit::EvolveResult   r = evolver.run();

        total_evals += r.evals;
        total_ms    += r.wall_ms;

        std::cout << "  ";
        if (r.success) {
            ++successes;
            std::cout << "✓ CONVERGIU";
        } else {
            std::cout << "✗ Não convergiu";
        }
        std::cout << " — evals=" << r.evals
                  << " fitness=" << r.fitness
                  << " (" << std::fixed << std::setprecision(1)
                  << r.wall_ms << " ms)\n";
    }

    // ── Sumário ───────────────────────────────────────────────────────────
    auto wall_end = std::chrono::steady_clock::now();
    double wall_total = std::chrono::duration<double, std::milli>(
        wall_end - wall_start).count();

    std::cout << "────────────────────────────────────────────────\n";
    if (n_runs > 1) {
        double rate = 100.0 * successes / n_runs;
        std::cout << "  Taxa de sucesso : " << successes << "/" << n_runs
                  << " (" << std::fixed << std::setprecision(0) << rate << "%)\n";
        std::cout << "  Evals médio     : " << (total_evals / n_runs) << "\n";
        std::cout << "  Tempo médio     : "
                  << std::fixed << std::setprecision(0)
                  << (total_ms / n_runs) << " ms/run\n";
    }
    std::cout << "  Tempo total     : "
              << std::fixed << std::setprecision(0)
              << wall_total << " ms\n\n";

    return (successes > 0) ? 0 : 1;
}

