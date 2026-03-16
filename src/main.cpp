// evo-circuit — ponto de entrada principal (placeholder)
// As fases seguintes adicionarão CLIConfig, io/ e o loop evolutivo aqui.
#include <iostream>
#include "evo-circuit/core/Parameters.hpp"

int main(int argc, char* argv[]) {
    evo_circuit::Parameters params;
    std::cout << "evo-circuit v0.1 — grid "
              << params.grid_rows << "x" << params.grid_cols
              << ", n_inputs=" << params.n_inputs
              << ", n_outputs=" << params.n_outputs
              << "\n";
    return 0;
}
