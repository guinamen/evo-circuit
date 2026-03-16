#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include "evo-circuit/evolution/BooleanEvolver.hpp"

namespace fs = std::filesystem;

static void print_help(const char* p) {
    std::cout
        << "Uso: " << p << " <arquivo.plu> [opções]\n\n"
        << "  --nodes N      Nós CGP           (padrão: 50)\n"
        << "  --lambda N     Filhos ES 1+λ      (padrão: 4)\n"
        << "  --evals N      Máx. avaliações    (padrão: 1000000)\n"
        << "  --mutation F   Taxa de mutação    (padrão: 0.05)\n"
        << "  --seed N       Seed (0=aleatória) (padrão: 42)\n"
        << "  --runs N       Runs independentes (padrão: 1)\n"
        << "  --verbose      Progresso detalhado\n"
        << "  --help         Esta ajuda\n\n"
        << "Exemplos:\n"
        << "  " << p << " data/plufiles/and2.plu\n"
        << "  " << p << " data/plufiles/xor2.plu --runs 5\n"
        << "  " << p << " data/plufiles/add1c.plu --nodes 100 --evals 5000000\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) { print_help(argv[0]); return 1; }

    std::string plu;
    evo_circuit::EvolverConfig cfg;
    int n_runs = 1;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a=="--help"||a=="-h")                 { print_help(argv[0]); return 0; }
        else if (a=="--verbose"||a=="-v")         { cfg.verbose=true; cfg.report_interval=10000; }
        else if (a=="--nodes"   && i+1<argc)      cfg.n_nodes       = std::stoi(argv[++i]);
        else if (a=="--lambda"  && i+1<argc)      cfg.lambda        = std::stoi(argv[++i]);
        else if (a=="--evals"   && i+1<argc)      cfg.max_evals     = std::stoll(argv[++i]);
        else if (a=="--mutation"&& i+1<argc)      cfg.mutation_rate = std::stof(argv[++i]);
        else if (a=="--seed"    && i+1<argc)      cfg.seed          = std::stoll(argv[++i]);
        else if (a=="--runs"    && i+1<argc)      n_runs            = std::stoi(argv[++i]);
        else if (a[0]!='-')                       plu = a;
        else { std::cerr << "Argumento desconhecido: " << a << "\n"; return 1; }
    }

    if (plu.empty()) { std::cerr << "Erro: arquivo .plu não especificado.\n"; return 1; }
    if (!fs::exists(plu)) { std::cerr << "Erro: arquivo não encontrado: " << plu << "\n"; return 1; }

    std::cout
        << "\n╔══════════════════════════════════════════════╗\n"
        << "║           evo-circuit v0.1                  ║\n"
        << "╚══════════════════════════════════════════════╝\n"
        << "  Benchmark   : " << fs::path(plu).filename().string() << "\n"
        << "  FunctionSet : AND OR XOR NOT NAND NOR XNOR\n"
        << "  Nós CGP     : " << cfg.n_nodes      << "\n"
        << "  λ (filhos)  : " << cfg.lambda        << "\n"
        << "  Máx. evals  : " << cfg.max_evals     << "\n"
        << "  Mutação     : " << cfg.mutation_rate << "\n"
        << "  Seed base   : " << cfg.seed          << "\n"
        << "  Runs        : " << n_runs            << "\n"
        << "────────────────────────────────────────────────\n";

    int successes = 0;
    long long total_evals = 0;
    double    total_ms    = 0.0;
    auto wall_start = std::chrono::steady_clock::now();

    for (int run = 0; run < n_runs; ++run) {
        auto run_cfg = cfg;
        run_cfg.seed = (cfg.seed==0) ? 0 : cfg.seed + run;
        if (n_runs > 1)
            std::cout << "\n  ── Run " << (run+1) << "/" << n_runs
                      << " (seed=" << run_cfg.seed << ") ──\n";

        evo_circuit::BooleanEvolver evolver(plu, run_cfg);
        evo_circuit::EvolveResult   r = evolver.run();
        total_evals += r.evals;
        total_ms    += r.wall_ms;

        std::cout << "  ";
        if (r.success) { ++successes; std::cout << "✓ CONVERGIU"; }
        else           { std::cout << "✗ Não convergiu"; }
        std::cout << " — evals=" << r.evals
                  << " fitness=" << r.fitness
                  << " (" << std::fixed << std::setprecision(1)
                  << r.wall_ms << " ms)\n";
    }

    auto wall_end = std::chrono::steady_clock::now();
    double wall_ms = std::chrono::duration<double, std::milli>(wall_end-wall_start).count();

    std::cout << "────────────────────────────────────────────────\n";
    if (n_runs > 1) {
        std::cout << "  Taxa de sucesso : " << successes << "/" << n_runs
                  << " (" << std::fixed << std::setprecision(0)
                  << (100.0*successes/n_runs) << "%)\n"
                  << "  Evals médio     : " << (total_evals/n_runs) << "\n"
                  << "  Tempo médio     : "
                  << std::fixed << std::setprecision(0)
                  << (total_ms/n_runs) << " ms/run\n";
    }
    std::cout << "  Tempo total     : "
              << std::fixed << std::setprecision(0) << wall_ms << " ms\n\n";

    return (successes > 0) ? 0 : 1;
}
