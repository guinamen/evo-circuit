#pragma once
// =============================================================================
// BooleanEvolver.hpp — ES (1+λ) para síntese de circuitos booleanos
//
// FIXES aplicados (histórico de diagnóstico):
//   - population_size setado ANTES de read_data() e finalize()
//     → sem esse fix, set_eval_chunk_size() usava tamanho inválido
//     → filhos não avaliados → fitness sempre igual ao pai inicial
//   - EvoCircuitInitializer usa FunctionSet próprio {AND,OR,XOR,NOT,NAND,NOR,XNOR}
//     em vez de FunctionsBoolean {AND,OR,NAND,NOR} do cgp-plusplus
//   - Sequência correta: setters → set_population_size → read_data()
//     → finalize → init_functions → init_composite → init_erc
//     → init_problem → init_checkpoint → init_algorithm → evolve
// =============================================================================

#include <string>
#include <utility>
#include <memory>
#include <chrono>
#include <stdexcept>

#include "composite/Composite.h"
#include "algorithm/OnePlusLambda.h"
#include "initializer/BlackBoxInitializer.h"
#include "problems/LogicSynthesisProblem.h"
#include "evo-circuit/evolution/EvoCircuitFunctions.hpp"

namespace evo_circuit {

struct EvolverConfig {
    int       n_nodes         = 10;   ///< 10 nós é suficiente para funções simples
    int       lambda          = 4;
    int       levels_back     = 10;   ///< levels_back=n_nodes: topologia livre
    float     mutation_rate   = 0.05f;
    long long max_evals       = 100'000LL; ///< 100k suficiente para and/xor/mux
    long long seed            = 42LL;
    bool      verbose         = false;
    int       report_interval = 10000;
};

struct EvolveResult {
    long long evals;
    int       fitness;
    bool      success;
    double    wall_ms;
};

// ── Inicializador com FunctionSet do evo-circuit ──────────────────────────
template<class E, class G, class F>
class EvoCircuitInitializer : public BlackBoxInitializer<E, G, F> {
public:
    explicit EvoCircuitInitializer(const std::string& plu)
        : BlackBoxInitializer<E, G, F>(plu) {}

    void init_functions() override {
        this->functions =
            std::make_shared<EvoCircuitFunctions<E>>(this->parameters);
    }

    void init_problem() override {
        auto problem = std::make_shared<LogicSynthesisProblem<E, G, F>>(
            this->parameters, this->evaluator,
            this->inputs, this->outputs,
            this->constants, this->num_instances);
        this->composite->set_problem(problem);
    }
};

// ── BooleanEvolver ────────────────────────────────────────────────────────
class BooleanEvolver {
public:
    explicit BooleanEvolver(const std::string& plu_file,
                            const EvolverConfig& cfg = {})
        : plu_file_(plu_file), cfg_(cfg)
    {}

    EvolveResult run() {
        auto t0 = std::chrono::steady_clock::now();

        EvoCircuitInitializer<unsigned long, int, int> init(plu_file_);
        auto p = init.get_parameters();

        // ── Configurar parâmetros ─────────────────────────────────────
        p->set_num_function_nodes(cfg_.n_nodes);
        p->set_num_functions(7);   // AND OR XOR NOT NAND NOR XNOR
        p->set_max_arity(2);
        p->set_num_constants(0);
        p->set_num_parents(1);
        p->set_num_offspring(cfg_.lambda);

        // FIX CRÍTICO: population_size deve ser setado ANTES de
        // finalize_parameter_configuration(), que chama
        // set_eval_chunk_size() — que asserta pop_size > 0.
        p->set_population_size(1 + cfg_.lambda);

        p->set_levels_back(cfg_.levels_back);
        p->set_mutation_rate(cfg_.mutation_rate);
        p->set_mutation_type(p->PROBABILISTIC_POINT_MUTATION);
        p->set_crossover_rate(0.0f);
        p->set_algorithm(p->ONE_PLUS_LAMBDA);
        p->set_problem(p->LOGIC_SYNTHESIS);
        p->set_max_fitness_evaluations(cfg_.max_evals);
        p->set_max_generations(cfg_.max_evals);
        p->set_ideal_fitness(0);
        p->set_minimizing_fitness(true);
        p->set_neutral_genetic_drift(true);
        p->set_evaluate_expression(false);
        p->set_num_jobs(1);
        p->set_num_eval_threads(1);
        p->set_write_statfile(false);
        p->set_checkpointing(false);
        p->set_report_simple(cfg_.verbose);
        p->set_report_during_job(cfg_.verbose);
        p->set_report_after_job(cfg_.verbose);
        p->set_report_interval(cfg_.report_interval);

        if (cfg_.seed > 0) {
            p->set_generate_random_seed(false);
            p->set_global_seed(cfg_.seed);
        } else {
            p->set_generate_random_seed(true);
        }

        // ── Sequência correta de inicialização ────────────────────────
        init.read_data();                        // seta num_inputs, num_outputs
        init.finalize_parameter_configuration(); // set_genome_size, eval_chunk_size
        init.init_functions();
        init.init_composite();
        init.init_erc();
        init.init_problem();
        init.init_checkpoint();
        init.init_algorithm();

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
