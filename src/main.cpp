#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <filesystem>

// Definir nível de compilação do spdlog ANTES do include
// Em Debug: habilitar TRACE; em Release: apenas DEBUG+
#ifdef NDEBUG
#  define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#else
#  define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#endif

#include "evo-circuit/core/Logger.hpp"
#include "evo-circuit/evolution/BooleanEvolver.hpp"

namespace fs = std::filesystem;

static void print_help(const char* p) {
    std::cout
        << "Uso: " << p << " <arquivo.plu> [opções]\n\n"
        << "Opções de circuito:\n"
        << "  --nodes N      Nós CGP           (padrão: 10)\n"
        << "  --lb N         Levels back        (padrão: igual a --nodes)\n"
        << "  --lambda N     Filhos ES 1+λ      (padrão: 4)\n"
        << "  --evals N      Máx. avaliações    (padrão: 100000)\n"
        << "  --mutation F   Taxa de mutação    (padrão: 0.05)\n"
        << "  --seed N       Seed (0=aleatória) (padrão: 42)\n"
        << "  --runs N       Runs independentes (padrão: 1)\n"
        << "  --verbose      Progresso a cada 5000 gerações\n\n"
        << "Opções de log:\n"
        << "  --log-level L  Nível: trace|debug|info|warn|error|off (padrão: info)\n"
        << "  --log-file F   Caminho do arquivo de log (padrão: somente console)\n\n"
        << "Exemplos:\n"
        << "  " << p << " data/plufiles/and2.plu --runs 10\n"
        << "  " << p << " data/plufiles/xor2.plu --log-level debug\n"
        << "  " << p << " data/plufiles/add1c.plu --nodes 50 --lb 50 --evals 2000000 \\\n"
        << "      --log-level debug --log-file logs/add1c.log\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) { print_help(argv[0]); return 1; }

    std::string plu, log_file;
    std::string log_level_str = "info";
    evo_circuit::EvolverConfig cfg;
    int n_runs = 1;
    bool lb_set = false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if      (a=="--help"||a=="-h")           { print_help(argv[0]); return 0; }
        else if (a=="--verbose"||a=="-v")        { cfg.verbose=true; cfg.report_interval=5000; }
        else if (a=="--nodes"   && i+1<argc)     { cfg.n_nodes=std::stoi(argv[++i]); }
        else if (a=="--lb"      && i+1<argc)     { cfg.levels_back=std::stoi(argv[++i]); lb_set=true; }
        else if (a=="--lambda"  && i+1<argc)     cfg.lambda=std::stoi(argv[++i]);
        else if (a=="--evals"   && i+1<argc)     cfg.max_evals=std::stoll(argv[++i]);
        else if (a=="--mutation"&& i+1<argc)     cfg.mutation_rate=std::stof(argv[++i]);
        else if (a=="--seed"    && i+1<argc)     cfg.seed=std::stoll(argv[++i]);
        else if (a=="--runs"    && i+1<argc)     n_runs=std::stoi(argv[++i]);
        else if (a=="--log-level" && i+1<argc)   log_level_str=argv[++i];
        else if (a=="--log-file"  && i+1<argc)   log_file=argv[++i];
        else if (a[0]!='-')                      plu=a;
        else { std::cerr<<"Argumento desconhecido: "<<a<<"\n"; return 1; }
    }

    if (!lb_set) cfg.levels_back = cfg.n_nodes;

    // ── Inicializar logger ────────────────────────────────────────────────
    auto log_level = evo_circuit::Logger::level_from_string(log_level_str);
    evo_circuit::Logger::init("evo-circuit", log_level, log_file);

    if (plu.empty()) {
        EVO_LOG_ERROR("Arquivo .plu não especificado");
        return 1;
    }
    if (!fs::exists(plu)) {
        EVO_LOG_ERROR("Arquivo não encontrado: {}", plu);
        return 1;
    }

    // ── Banner ────────────────────────────────────────────────────────────
    std::cout
        << "\n╔══════════════════════════════════════════════╗\n"
        << "║           evo-circuit v0.1                  ║\n"
        << "╚══════════════════════════════════════════════╝\n"
        << "  Benchmark   : " << fs::path(plu).filename().string() << "\n"
        << "  FunctionSet : AND OR XOR NOT NAND NOR XNOR\n"
        << "  Nós CGP     : " << cfg.n_nodes        << "\n"
        << "  Levels back : " << cfg.levels_back     << "\n"
        << "  λ (filhos)  : " << cfg.lambda          << "\n"
        << "  Máx. evals  : " << cfg.max_evals       << "\n"
        << "  Mutação     : " << cfg.mutation_rate   << "\n"
        << "  Seed base   : " << cfg.seed            << "\n"
        << "  Runs        : " << n_runs              << "\n"
        << "  Log level   : " << log_level_str       << "\n";
    if (!log_file.empty())
        std::cout << "  Log file    : " << log_file << "\n";
    std::cout << "────────────────────────────────────────────────\n";

    EVO_LOG_INFO("evo-circuit iniciado: benchmark={} runs={} seed={}",
        fs::path(plu).filename().string(), n_runs, cfg.seed);

    // ── Loop de runs ──────────────────────────────────────────────────────
    int successes = 0;
    long long total_evals = 0;
    double    total_ms    = 0.0;
    auto wall_start = std::chrono::steady_clock::now();

    for (int run = 0; run < n_runs; ++run) {
        auto rc = cfg;
        rc.seed = (cfg.seed == 0) ? 0 : cfg.seed + run;

        EVO_LOG_INFO("=== Run {}/{} (seed={}) ===", run+1, n_runs, rc.seed);
        if (n_runs > 1)
            std::cout << "\n  ── Run " << (run+1) << "/" << n_runs
                      << " (seed=" << rc.seed << ") ──\n";

        try {
            evo_circuit::BooleanEvolver evolver(plu, rc);
            auto r = evolver.run();
            total_evals += r.evals;
            total_ms    += r.wall_ms;

            std::cout << "  ";
            if (r.success) {
                ++successes;
                std::cout << "✓ CONVERGIU";
                EVO_LOG_INFO("Run {}: SUCESSO evals={} fitness=0 wall={:.1f}ms",
                    run+1, r.evals, r.wall_ms);
            } else {
                std::cout << "✗ Não convergiu";
                EVO_LOG_WARN("Run {}: FALHOU fitness={} evals={} wall={:.1f}ms",
                    run+1, r.fitness, r.evals, r.wall_ms);
            }
            std::cout << " — evals=" << r.evals
                      << "  fitness=" << r.fitness
                      << "  (" << std::fixed << std::setprecision(1)
                      << r.wall_ms << " ms)\n";

        } catch (const std::exception& e) {
            EVO_LOG_ERROR("Run {} falhou com exceção: {}", run+1, e.what());
            std::cerr << "  ERRO: " << e.what() << "\n";
        }
    }

    // ── Sumário ───────────────────────────────────────────────────────────
    auto wall_end = std::chrono::steady_clock::now();
    double wall_ms = std::chrono::duration<double,std::milli>(wall_end-wall_start).count();

    std::cout << "────────────────────────────────────────────────\n";
    if (n_runs > 1) {
        double rate = 100.0 * successes / n_runs;
        std::cout
            << "  Taxa de sucesso : " << successes << "/" << n_runs
            << " (" << std::fixed << std::setprecision(0) << rate << "%)\n"
            << "  Evals médio     : " << (total_evals / n_runs) << "\n"
            << "  Tempo médio     : "
            << std::fixed << std::setprecision(1)
            << (total_ms / n_runs) << " ms/run\n";
        EVO_LOG_INFO("Sumário: {}/{} runs bem-sucedidas ({:.0f}%) — evals_médio={} tempo_total={:.0f}ms",
            successes, n_runs, rate, total_evals/n_runs, wall_ms);
    }
    std::cout << "  Tempo total     : "
              << std::fixed << std::setprecision(1)
              << wall_ms << " ms\n\n";

    evo_circuit::Logger::flush();
    return (successes > 0) ? 0 : 1;
}
