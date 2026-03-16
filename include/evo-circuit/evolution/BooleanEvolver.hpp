#pragma once
// =============================================================================
// BooleanEvolver.hpp — ES (1+λ) para síntese de circuitos booleanos
//
// ORDEM CORRETA de inicialização (crítica):
//   1. read_data()                  — lê .plu, seta num_variables e num_outputs
//   2. init_comandline_parameters() — usa num_outputs; chama set_genome_size()
//   3. init_functions()
//   4. init_composite()
//   5. init_problem()
//   6. init_algorithm()
//   7. get_algorithm()->evolve()
// =============================================================================

#include <string>
#include <utility>
#include <memory>
#include <iostream>
#include <chrono>

#include "composite/Composite.h"
#include "algorithm/OnePlusLambda.h"
#include "initializer/LogicSynthesisInitializer.h"

namespace evo_circuit {

struct EvolverConfig {
    int       n_nodes         = 50;
    int       lambda          = 4;
    int       levels_back     = 50;
    float     mutation_rate   = 0.05f;
    long long max_evals       = 1'000'000LL;
    long long seed            = 42LL;
    bool      verbose         = true;
    int       report_interval = 10000;
};

struct EvolveResult {
    long long evals;
    int       fitness;
    bool      success;
    double    wall_ms;
};

class BooleanEvolver {
public:
    explicit BooleanEvolver(const std::string& plu_file,
                            const EvolverConfig& cfg = {})
        : plu_file_(plu_file), cfg_(cfg)
    {}

    EvolveResult run() {
        auto t0 = std::chrono::steady_clock::now();

        LogicSynthesisInitializer<unsigned long, int, int> init(plu_file_);

        // Flags que não dependem do .plu — configurar antes de qualquer chamada
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

        // ── PASSO 1: read_data() OBRIGATORIAMENTE PRIMEIRO ────────────
        // Lê .plu → chama set_num_variables() + set_num_outputs()
        // Sem isso, set_genome_size() dentro de init_comandline_parameters
        // falha com assert(num_outputs > 0)
        init.read_data();

        // ── PASSO 2: init_comandline_parameters ────────────────────────
        // n_variables=-1 e n_outputs=-1: não sobrescreve valores do .plu
        init.init_comandline_parameters(
            /*algorithm*/          0,
            /*n_nodes*/            cfg_.n_nodes,
            /*n_variables*/       -1,
            /*n_constants*/        0,
            /*n_outputs*/         -1,
            /*n_functions*/        4,
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

        // ── PASSOS 3–6 ─────────────────────────────────────────────────
        init.init_functions();
        init.init_composite();
        init.init_erc();
        init.init_problem();
        init.init_algorithm();

        // ── PASSO 7: evoluir ───────────────────────────────────────────
        std::pair<int, int> result = init.get_algorithm()->evolve();

        auto t1 = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        return EvolveResult{
            static_cast<long long>(result.first),
            result.second,
            result.second == 0,
            ms
        };
    }

private:
    std::string   plu_file_;
    EvolverConfig cfg_;
};

} // namespace evo_circuit
