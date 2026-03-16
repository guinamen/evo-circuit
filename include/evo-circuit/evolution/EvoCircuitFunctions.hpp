#pragma once
// =============================================================================
// EvoCircuitFunctions.hpp — FunctionSet completo do evo-circuit
//
// Implementa os operadores CGP conforme o documento de arquitetura (seção 4):
//   0=AND  1=OR  2=XOR  3=NOT  4=NAND  5=NOR  6=XNOR
//
// Derivado de Functions<E> do cgp-plusplus para integração com o Evaluator.
// A presença de XOR é crítica: permite que o CGP resolva funções de paridade
// e somadores sem precisar de dezenas de nós NAND encadeados.
// =============================================================================

#include "functions/Functions.h"
#include <stdexcept>
#include <string>

template<class E>
class EvoCircuitFunctions : public Functions<E> {
public:
    explicit EvoCircuitFunctions(std::shared_ptr<Parameters> p)
        : Functions<E>(p) {}

    E call_function(E inputs[], int f) override {
        switch (f) {
            case 0: return inputs[0] & inputs[1];           // AND
            case 1: return inputs[0] | inputs[1];           // OR
            case 2: return inputs[0] ^ inputs[1];           // XOR
            case 3: return ~inputs[0];                      // NOT
            case 4: return ~(inputs[0] & inputs[1]);        // NAND
            case 5: return ~(inputs[0] | inputs[1]);        // NOR
            case 6: return ~(inputs[0] ^ inputs[1]);        // XNOR
            default:
                throw std::invalid_argument(
                    "EvoCircuitFunctions: ID de função inválido: " +
                    std::to_string(f));
        }
    }

    std::string function_name(int f) override {
        constexpr const char* names[] =
            {"AND", "OR", "XOR", "NOT", "NAND", "NOR", "XNOR"};
        if (f >= 0 && f < 7) return names[f];
        return "?";
    }

    std::string input_name(int i) override {
        return "x" + std::to_string(i);
    }

    /// NOT tem aridade 1; todas as outras têm aridade 2.
    int arity_of(int f) override {
        return (f == 3) ? 1 : 2;
    }
};
