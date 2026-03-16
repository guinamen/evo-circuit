#pragma once
// =============================================================================
// BooleanFitness.hpp — wrapper reentrante sobre LogicSynthesisProblem
//
// O cgp-plusplus já implementa a distância de Hamming em LogicSynthesisProblem.
// Este header expõe uma interface leve usada pelo BooleanEvolver para calcular
// o fitness de um indivíduo dado um vetor de entradas e saídas esperadas.
//
// Reentrância: cada thread cria sua própria instância — sem estado compartilhado.
// =============================================================================

#include <vector>
#include <cstdint>
#include <cmath>
#include <stdexcept>

namespace evo_circuit {

/// Fitness booleano via distância de Hamming (minimizar).
/// Valor 0 = circuito correto para todas as entradas da truth table.
struct BooleanFitness {

    /// Calcula a distância de Hamming entre saídas reais e esperadas.
    /// outputs_actual e outputs_expected têm tamanho n_outputs,
    /// cada elemento é um bitmask com n_patterns bits.
    static int hamming_distance(
        const std::vector<unsigned long>& outputs_actual,
        const std::vector<unsigned long>& outputs_expected,
        int n_patterns)
    {
        if (outputs_actual.size() != outputs_expected.size())
            throw std::invalid_argument("BooleanFitness: tamanho de outputs diverge");

        int errors = 0;
        for (size_t i = 0; i < outputs_actual.size(); ++i) {
            unsigned long diff = outputs_actual[i] ^ outputs_expected[i];
            // contar bits diferentes nos primeiros n_patterns bits
            for (int b = 0; b < n_patterns; ++b)
                if ((diff >> b) & 1UL) ++errors;
        }
        return errors;
    }

    /// Fitness normalizado [0.0, 1.0]: 1.0 = perfeito, 0.0 = pior caso.
    static double normalized(int hamming, int total_bits) {
        if (total_bits == 0) return 1.0;
        return 1.0 - static_cast<double>(hamming) / total_bits;
    }
};

} // namespace evo_circuit
