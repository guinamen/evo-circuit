#pragma once
#include <cstdint>

namespace evo_circuit {

/// IDs de função para nós primitivos (0..127) e LUT-k (128+).
/// Bits [7:6] do ID codificam k-2 para LUT-k:
///   00 = LUT-2, 01 = LUT-3, 10 = LUT-4, 11 = LUT-5
///   IDs >= 192 reservados para LUT-6
namespace FunctionID {
    // Primitivos padrão
    constexpr uint32_t AND  = 0;
    constexpr uint32_t OR   = 1;
    constexpr uint32_t XOR  = 2;
    constexpr uint32_t NOT  = 3;
    constexpr uint32_t NAND = 4;
    constexpr uint32_t NOR  = 5;
    constexpr uint32_t XNOR = 6;
    constexpr uint32_t MUX  = 7;

    // LUT-k (IDs >= 128)
    constexpr uint32_t LUT2_BASE = 128;
    constexpr uint32_t LUT3_BASE = 128 + 64;   // bits[7:6] = 01
    constexpr uint32_t LUT4_BASE = 128 + 128;  // bits[7:6] = 10
    constexpr uint32_t LUT5_BASE = 128 + 192;  // bits[7:6] = 11 — aproximado
    constexpr uint32_t LUT6_BASE = 192;
}

/// Utilitários para inspeção de IDs de função.
struct LutFunctionSet {
    static bool is_lut(uint32_t f) { return f >= 128; }

    static uint8_t lut_arity(uint32_t f) {
        if (!is_lut(f)) return 0;
        if (f >= 192) return 6;
        uint8_t enc = static_cast<uint8_t>((f >> 6) & 0x03);
        return static_cast<uint8_t>(enc + 2);
    }
};

} // namespace evo_circuit
