#pragma once
// =============================================================================
// BooleanEvolver.hpp — ES (1+λ) para síntese de circuitos booleanos
//
// Pipeline de inicialização do cgp-plusplus:
//   1. LogicSynthesisInitializer(plu_file)
//   2. init_comandline_parameters(...)   → configura Parameters + finaliza
//   3. read_data()                       → lê .plu, detecta n_inputs/n_outputs
//   4. init_functions()                  → cria FunctionsBoolean {AND,OR,NAND,NOR}
//   5. init_composite()                  → cria Composite (Random, Species, etc.)
//   6. init_problem()                    → cria LogicSynthesisProblem
//   7. init_algorithm()                  → cria OnePlusLambda
//   8. get_algorithm()->evolve()         → executa ES, retorna pair<int,F>
//
// Nota: Checkpoint.h deve ser incluído ANTES de Composite.h porque
// Composite.h referencia Checkpoint<E,G,F> sem incluir o header.
// =============================================================================

#include <string>
#include <utility>
#include <memory>
#include <iostream>
#include <chrono>
#include <stdexcept>

// Ordem importa: Checkpoint antes de Composite
#include "checkpoint/Checkpoint.h"
#include "composite/Composite.h"
#include "algorithm/OnePlusLambda.h"
#include "initializer/LogicSynthesisInitializer.h"

namespace evo_circuit {

/// Parâmetros de configuração do evolver booleano.
struct EvolverConfig {
    int       n_nodes         = 50;
    int       lambda          = 4;      ///< filhos por geração (ES 1+λ)
    int       levels_back     = 50;
    float     mutation_rate   = 0.05f;
    long long max_evals       = 1'000'000LL;
    long long seed            = 42LL;   ///< > 0 = seed fixa; 0 = aleatória
    bool      verbose         = true;
    int       report_interval = 10000;
};

/// Resultado de uma run evolutiva.
struct EvolveResult {
    long long evals;      ///< avaliações de fitness consumidas
    int       fitness;    ///< distância de Hamming final (0 = perfeito)
    bool      success;
    double    wall_ms;
};

/// Evolver booleano — encapsula o pipeline completo do cgp-plusplus.
class BooleanEvolver {
public:
    explicit BooleanEvolver(const std::string& plu_file,
                            const EvolverConfig& cfg = {})
        : plu_file_(plu_file), cfg_(cfg)
    {}

    EvolveResult run() {
        auto t0 = std::chrono::steady_clock::now();

        // ── 1. Criar inicializador ─────────────────────────────────────
        LogicSynthesisInitializer<unsigned long, int, int> init(plu_file_);

        // ── 2. Configurar parâmetros ───────────────────────────────────
        // n_variables e n_outputs são -1 aqui: read_data() os sobrescreve
        // com os valores do cabeçalho .i/.o do arquivo .plu.
        auto params = init.get_parameters();
        params->set_neutral_genetic_drift(true);
        params->set_evaluate_expression(false);
        params->set_minimizing_fitness(true);
        params->set_report_simple(cfg_.verbose);
        params->set_report_during_job(cfg_.verbose);
        params->set_report_after_job(cfg_.verbose);
        params->set_report_interval(cfg_.report_interval);
        params->set_write_statfile(false);
        params->set_checkpointing(false);

        if (cfg_.seed > 0) {
            params->set_generate_random_seed(false);
            params->set_global_seed(cfg_.seed);
        } else {
            params->set_generate_random_seed(true);
        }

        // init_comandline_parameters finaliza a configuração (chama
        // finalize_parameter_configuration internamente).
        init.init_comandline_parameters(
            /*algorithm*/          0,               // ONE_PLUS_LAMBDA
            /*n_nodes*/            cfg_.n_nodes,
            /*n_variables*/       -1,               // lido do .plu
            /*n_constants*/        0,
            /*n_outputs*/         -1,               // lido do .plu
            /*n_functions*/        4,               // AND OR NAND NOR
            /*max_arity*/          2,
            /*n_parents*/          1,
            /*n_offspring*/        cfg_.lambda,
            /*mutation_rate*/      cfg_.mutation_rate,
            /*max_fitness_evals*/  cfg_.max_evals,
            /*ideal_fitness*/      0,
            /*n_jobs*/             1,
            /*global_seed*/        cfg_.seed > 0 ? cfg_.seed : -1LL,
            /*duplication_rate*/  -1.0f,
            /*max_dup_depth*/     -1,
            /*inversion_rate*/    -1.0f,
            /*max_inv_depth*/     -1,
            /*crossover_rate*/     0.0f,
            /*levels_back*/        cfg_.levels_back
        );

        // ── 3. Ler .plu (detecta n_inputs e n_outputs) ────────────────
        init.read_data();

        // ── 4–7. Pipeline de inicialização ─────────────────────────────
        init.init_functions();
        init.init_composite();
        init.init_problem();
        init.init_algorithm();

        // ── 8. Executar ES ─────────────────────────────────────────────
        // Captura em std::pair explícito — evita ambiguidade com
        // structured binding sobre retorno de método virtual em shared_ptr.
        std::pair<int, int> result = init.get_algorithm()->evolve();

        auto t1 = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        int evals       = result.first;
        int best_fitness = result.second;

        return EvolveResult{
            static_cast<long long>(evals),
            best_fitness,
            best_fitness == 0,
            ms
        };
    }

private:
    std::string   plu_file_;
    EvolverConfig cfg_;
};

} // namespace evo_circuit
