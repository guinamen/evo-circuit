#pragma once
// =============================================================================
// BooleanEvolver.hpp — ES (1+λ) para síntese de circuitos booleanos
//
// Sequência de inicialização do cgp-plusplus (Initializer API):
//   1. LogicSynthesisInitializer(plu_file)  — seta benchmark_file
//   2. init_comandline_parameters(...)       — configura Parameters + finaliza
//   3. read_data()                           — lê .plu, seta n_inputs/n_outputs
//   4. init_functions()                      — cria FunctionsBoolean
//   5. init_composite()                      — cria Composite (Random, Species, etc.)
//   6. init_problem()                        — cria LogicSynthesisProblem
//   7. init_algorithm()                      — cria OnePlusLambda
//   8. get_algorithm()->evolve()             — executa ES
// =============================================================================

#include <string>
#include <utility>
#include <memory>
#include <iostream>
#include <chrono>

// Checkpoint.h DEVE vir antes de Composite.h — Composite.h referencia
// Checkpoint<E,G,F> sem incluir o header por conta própria.
#include "checkpoint/Checkpoint.h"
#include "composite/Composite.h"
#include "algorithm/OnePlusLambda.h"
#include "initializer/LogicSynthesisInitializer.h"

namespace evo_circuit {

/// Parâmetros de configuração do evolver booleano.
struct EvolverConfig {
    int       n_nodes         = 50;
    int       lambda          = 4;      ///< filhos (num_offspring); num_parents = 1
    int       levels_back     = 50;
    float     mutation_rate   = 0.05f;
    long long max_evals       = 1'000'000LL; ///< max fitness evaluations
    long long seed            = 42LL;   ///< > 0 = seed fixa; 0 = aleatória
    bool      verbose         = true;
    int       report_interval = 10000;
};

/// Resultado de uma run evolutiva.
struct EvolveResult {
    long long evals;      ///< avaliações de fitness consumidas
    int       fitness;    ///< distância de Hamming final (0 = sucesso)
    bool      success;
    double    wall_ms;
};

/// Evolver booleano — encapsula o pipeline completo do cgp-plusplus.
class BooleanEvolver {
public:
    BooleanEvolver(const std::string& plu_file, const EvolverConfig& cfg = {})
        : plu_file_(plu_file), cfg_(cfg)
    {}

    EvolveResult run() {
        auto t0 = std::chrono::steady_clock::now();

        // ── 1. Criar inicializador ─────────────────────────────────────
        LogicSynthesisInitializer<unsigned long, int, int> init(plu_file_);

        // ── 2. Configurar parâmetros via init_comandline_parameters ────
        // Assinatura: (algorithm, n_nodes, n_variables, n_constants,
        //              n_outputs, n_functions, max_arity, n_parents,
        //              n_offspring, mutation_rate, max_fitness_evals,
        //              ideal_fitness, n_jobs, global_seed,
        //              duplication_rate, max_duplication_depth,
        //              inversion_rate, max_inversion_depth,
        //              crossover_rate, levels_back)
        //
        // n_variables e n_outputs são sobrescritos por read_data() com
        // os valores do arquivo .plu — passamos -1 para não forçar aqui.
        // n_functions=4 {AND,OR,NAND,NOR}, max_arity=2 (FunctionsBoolean).
        auto* p = init.get_parameters().get();
        p->set_neutral_genetic_drift(true);
        p->set_evaluate_expression(false);
        p->set_minimizing_fitness(true);
        p->set_report_simple(cfg_.verbose);
        p->set_report_during_job(cfg_.verbose);
        p->set_report_after_job(cfg_.verbose);
        p->set_report_interval(cfg_.report_interval);
        p->set_write_statfile(false);
        p->set_checkpointing(false);
        p->set_problem(p->LOGIC_SYNTHESIS);
        p->set_algorithm(p->ONE_PLUS_LAMBDA);

        if (cfg_.seed > 0) {
            p->set_generate_random_seed(false);
            p->set_global_seed(cfg_.seed);
        } else {
            p->set_generate_random_seed(true);
        }

        init.init_comandline_parameters(
            /*algorithm*/          0,          // ONE_PLUS_LAMBDA
            /*n_nodes*/            cfg_.n_nodes,
            /*n_variables*/       -1,          // lido do .plu
            /*n_constants*/        0,
            /*n_outputs*/         -1,          // lido do .plu
            /*n_functions*/        4,          // AND OR NAND NOR
            /*max_arity*/          2,
            /*n_parents*/          1,
            /*n_offspring*/        cfg_.lambda,
            /*mutation_rate*/      cfg_.mutation_rate,
            /*max_fitness_evals*/  cfg_.max_evals,
            /*ideal_fitness*/      0,          // Hamming=0 → solução perfeita
            /*n_jobs*/             1,
            /*global_seed*/        cfg_.seed > 0 ? cfg_.seed : -1LL,
            /*duplication_rate*/  -1.0f,
            /*max_dup_depth*/     -1,
            /*inversion_rate*/    -1.0f,
            /*max_inv_depth*/     -1,
            /*crossover_rate*/     0.0f,
            /*levels_back*/        cfg_.levels_back
        );

        // ── 3. Ler dados do .plu (seta n_inputs e n_outputs) ──────────
        init.read_data();

        // ── 4–7. Pipeline de inicialização ─────────────────────────────
        init.init_functions();
        init.init_composite();
        init.init_problem();
        init.init_algorithm();

        // ── 8. Evoluir ─────────────────────────────────────────────────
        auto algo = init.get_algorithm();
        auto [evals, best_fitness] = algo->evolve();

        auto t1 = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        return EvolveResult{
            evals,
            static_cast<int>(best_fitness),
            best_fitness == 0,
            ms
        };
    }

private:
    std::string   plu_file_;
    EvolverConfig cfg_;
};

} // namespace evo_circuit
