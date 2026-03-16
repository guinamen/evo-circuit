#pragma once
// =============================================================================
// BooleanEvolver.hpp — ES (1+λ) para síntese de circuitos booleanos
//
// Usa o cgp-plusplus como motor CGP:
//   - Parameters (cgp-plusplus)  → configuração do grid e da ES
//   - FunctionsBoolean<ulong>    → function set {AND, OR, NAND, NOR}
//   - LogicSynthesisInitializer  → carrega .plu e monta Composite
//   - OnePlusLambda              → loop evolutivo ES (1+λ)
//
// Interface pública:
//   BooleanEvolver evolver("data/plufiles/epar3.plu", params);
//   auto result = evolver.run();   // retorna {gerações, fitness_final}
// =============================================================================

#include <string>
#include <utility>
#include <memory>
#include <iostream>
#include <chrono>

// cgp-plusplus headers
#include "parameters/Parameters.h"
#include "functions/BooleanFunctions.h"
#include "composite/Composite.h"
#include "algorithm/OnePlusLambda.h"
#include "initializer/LogicSynthesisInitializer.h"

namespace evo_circuit {

/// Parâmetros de configuração do evolver booleano.
struct EvolverConfig {
    int    n_nodes        = 50;     ///< número de nós funcionais no grid
    int    n_inputs       = 3;      ///< entradas do circuito
    int    n_outputs      = 1;      ///< saídas do circuito
    int    lambda         = 4;      ///< filhos por geração (ES 1+λ)
    int    levels_back    = 50;     ///< conectividade retroativa (0 = irrestrito)
    float  mutation_rate  = 0.05f;  ///< taxa de mutação pontual
    long long max_generations = 1'000'000LL;
    long long seed        = 42LL;   ///< 0 = seed aleatória
    bool   verbose        = true;
    int    report_interval = 10000; ///< gerações entre relatórios
};

/// Resultado de uma run evolutiva.
struct EvolveResult {
    long long generations;
    int       fitness;      ///< distância de Hamming (0 = sucesso)
    bool      success;
    double    wall_ms;
};

/// Evolver booleano: encapsula o cgp-plusplus para síntese de circuitos.
class BooleanEvolver {
public:
    BooleanEvolver(const std::string& plu_file, const EvolverConfig& cfg = {})
        : plu_file_(plu_file), cfg_(cfg)
    {}

    /// Executa uma run evolutiva e retorna o resultado.
    EvolveResult run() {
        auto t0 = std::chrono::steady_clock::now();

        // ── Configurar Parameters do cgp-plusplus ──────────────────────
        auto params = std::make_shared<Parameters>();

        params->set_num_function_nodes(cfg_.n_nodes);
        params->set_num_inputs(cfg_.n_inputs);
        params->set_num_outputs(cfg_.n_outputs);
        params->set_max_arity(2);       // FunctionsBoolean tem aridade 2
        params->set_num_functions(4);   // AND, OR, NAND, NOR
        params->set_lambda(cfg_.lambda);
        params->set_levels_back(cfg_.levels_back == 0 ? cfg_.n_nodes : cfg_.levels_back);
        params->set_mutation_rate(cfg_.mutation_rate);
        params->set_mutation_type(params->PROBABILISTIC_POINT_MUTATION);
        params->set_max_generations(cfg_.max_generations);
        params->set_ideal_fitness(0);      // distância Hamming = 0 → solução perfeita
        params->set_minimizing_fitness(true);
        params->set_neutral_genetic_drift(true);
        params->set_evaluate_expression(false);
        params->set_report_simple(cfg_.verbose);
        params->set_report_during_job(cfg_.verbose);
        params->set_report_interval(cfg_.report_interval);
        params->set_report_after_job(cfg_.verbose);
        params->set_write_statfile(false);
        params->set_checkpointing(false);
        params->set_problem(params->LOGIC_SYNTHESIS);
        params->set_algorithm(params->ONE_PLUS_LAMBDA);
        params->set_genome_size(); // recalcula após setar n_nodes/arity/outputs

        if (cfg_.seed == 0)
            params->set_generate_random_seed(true);
        else {
            params->set_generate_random_seed(false);
            params->set_global_seed(cfg_.seed);
        }

        // ── Inicializar via LogicSynthesisInitializer ──────────────────
        LogicSynthesisInitializer<unsigned long, int, int> initializer(plu_file_);
        initializer.init(params);

        auto composite = initializer.get_composite();

        // ── Executar ES (1+λ) ──────────────────────────────────────────
        OnePlusLambda<unsigned long, int, int> es(composite);
        auto [evals, best_fitness] = es.evolve();

        auto t1 = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        EvolveResult result;
        result.generations = evals;
        result.fitness      = static_cast<int>(best_fitness);
        result.success      = (best_fitness == 0);
        result.wall_ms      = ms;

        return result;
    }

private:
    std::string   plu_file_;
    EvolverConfig cfg_;
};

} // namespace evo_circuit
