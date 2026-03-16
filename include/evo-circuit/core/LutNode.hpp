#pragma once
#include <bitset>
#include <cstdint>
#include <cassert>

namespace evo_circuit {

/// Nó LUT-k: tabela verdade de 2^k bits, aridade variável k ∈ {2..6}.
/// A tabela verdade é parte do genótipo e sujeita a mutação independente.
struct LutNode {
    uint8_t k = 2;                  ///< aridade (número de entradas)
    std::bitset<64> truth_table;    ///< 2^k bits válidos; LSB = índice 0b000...0

    /// Avalia o nó dado um índice de entrada (os k bits menos significativos de idx).
    bool evaluate(uint32_t input_idx) const {
        assert(k >= 2 && k <= 6);
        assert(input_idx < (1u << k));
        return truth_table.test(input_idx);
    }

    /// Número de entradas válidas: 2^k
    uint32_t table_size() const { return 1u << k; }
};

} // namespace evo_circuit
