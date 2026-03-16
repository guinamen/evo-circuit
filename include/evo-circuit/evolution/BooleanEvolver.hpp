#pragma once
// =============================================================================
// BooleanEvolver.hpp — ES (1+λ) com logging spdlog integrado
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
#include "evo-circuit/core/Logger.hpp"

namespace evo_circuit {

struct EvolverConfig {
    int       n_nodes         = 10;
    int       lambda          = 4;
    int       levels_back     = 10;
    float     mutation_rate   = 0.05f;
    long long max_evals       = 100'000LL;
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
        EVO_LOG_DEBUG("FunctionSet: AND OR XOR NOT NAND NOR XNOR (7 funções)");
    }

    void init_problem() override {
        auto problem = std::make_shared<LogicSynthesisProblem<E, G, F>>(
            this->parameters, this->evaluator,
            this->inputs, this->outputs,
            this->constants, this->num_instances);
        this->composite->set_problem(problem);
        EVO_LOG_DEBUG("LogicSynthesisProblem criado: {} instâncias, num_bits={}",
            this->num_instances,
            (int)std::pow(2, this->parameters->get_num_inputs()));
    }
};

// ── Listener de geração para log detalhado ───────────────────────────────
// Instrumentação via wrapper do OnePlusLambda com log a cada N gerações.
template<class E, class G, class F>
class LoggingES : public OnePlusLambda<E, G, F> {
public:
    explicit LoggingES(std::shared_ptr<Composite<E, G, F>> composite,
                       int log_every = 1000)
        : OnePlusLambda<E, G, F>(composite)
        , log_every_(log_every)
    {}

    std::pair<int, F> evolve() override {
        EVO_LOG_INFO("Iniciando ES (1+{}) — max_evals={} mut={:.3f} lb={}",
            this->parameters->get_lambda(),
            this->parameters->get_max_fitness_evaluations(),
            this->parameters->get_mutation_rate(),
            this->parameters->get_levels_back());
        EVO_LOG_INFO("Grid: {} nós funcionais, genome_size={}",
            this->parameters->get_num_function_nodes(),
            this->parameters->get_genome_size());
        EVO_LOG_INFO("Circuito: {} entradas, {} saídas",
            this->parameters->get_num_inputs(),
            this->parameters->get_num_outputs());

        prev_fitness_ = std::numeric_limits<int>::max();
        stagnation_   = 0;

        auto result = OnePlusLambda<E, G, F>::evolve();

        F best = result.second;
        if (best == 0) {
            EVO_LOG_INFO("✓ CONVERGIU — evals={} fitness=0", result.first);
        } else {
            EVO_LOG_WARN("✗ Não convergiu após {} avaliações — fitness residual={}",
                result.first, best);
        }
        return result;
    }

protected:
    // Sobrescrever report para capturar progresso via spdlog
    void report(int gen) override {
        // Chamar o report original (imprime "Generation # N :: Best Fitness: F")
        // apenas se verbose estiver habilitado no cgp-plusplus
        OnePlusLambda<E, G, F>::report(gen);

        if (gen % log_every_ == 0) {
            F cur = this->best_fitness;
            int delta = static_cast<int>(prev_fitness_) - static_cast<int>(cur);

            if (delta > 0) {
                EVO_LOG_DEBUG("gen={:>8} | fitness={:>4} | melhora={:>+3} | stagnação={}",
                    gen, (int)cur, delta, stagnation_);
                stagnation_ = 0;
            } else {
                ++stagnation_;
                if (stagnation_ % 10 == 0) {
                    EVO_LOG_WARN("gen={:>8} | fitness={:>4} | estagnado há {} intervalos",
                        gen, (int)cur, stagnation_);
                }
            }
            prev_fitness_ = cur;
        }
    }

private:
    int log_every_;
    F   prev_fitness_;
    int stagnation_ = 0;
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

        EVO_LOG_INFO("─────────────────────────────────────");
        EVO_LOG_INFO("Benchmark: {}", plu_file_);
        EVO_LOG_INFO("Config: n={} lb={} λ={} mut={:.3f} seed={} max_evals={}",
            cfg_.n_nodes, cfg_.levels_back, cfg_.lambda,
            cfg_.mutation_rate, cfg_.seed, cfg_.max_evals);

        EvoCircuitInitializer<unsigned long, int, int> init(plu_file_);
        auto p = init.get_parameters();

        p->set_num_function_nodes(cfg_.n_nodes);
        p->set_num_functions(7);
        p->set_max_arity(2);
        p->set_num_constants(0);
        p->set_num_parents(1);
        p->set_num_offspring(cfg_.lambda);
        p->set_population_size(1 + cfg_.lambda);   // FIX: antes de finalize
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

        EVO_LOG_DEBUG("Lendo arquivo de spec: {}", plu_file_);
        init.read_data();

        EVO_LOG_DEBUG("Spec lida: num_inputs={} num_outputs={} instâncias={}",
            p->get_num_inputs(), p->get_num_outputs(), 1);

        init.finalize_parameter_configuration();

        EVO_LOG_DEBUG("Parâmetros finalizados: genome_size={} pop_size={} eval_chunk={}",
            p->get_genome_size(), p->get_population_size(), p->get_eval_chunk_size());

        init.init_functions();
        init.init_composite();
        init.init_erc();
        init.init_problem();
        init.init_checkpoint();
        init.init_algorithm();

        EVO_LOG_DEBUG("Pipeline inicializado — iniciando evolução");

        std::pair<int, int> result = init.get_algorithm()->evolve();

        auto t1 = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        EVO_LOG_INFO("Resultado: evals={} fitness={} wall={:.1f}ms",
            result.first, result.second, ms);

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
