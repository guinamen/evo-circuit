# evo-circuit

Framework industrial em C++17 para síntese evolutiva de circuitos digitais
via Cartesian Genetic Programming (CGP).

Documento de arquitetura: `evo_circuit_arquitetura_v7.docx`

## Dependências

| Dependência     | Versão mín. | Origem            | Licença     |
|-----------------|-------------|-------------------|-------------|
| CMake           | 3.20        | host              | —           |
| GCC / Clang     | C++17       | host              | —           |
| OpenMP          | —           | host              | —           |
| cgp-plusplus    | latest      | submodule         | AFL 3.0     |
| SQLiteCpp       | latest      | submodule         | MIT         |
| xsimd           | latest      | submodule         | MIT         |
| LLVM OrcJIT     | 17+         | host (opcional)   | Apache 2.0  |
| Catch2          | 3.x         | FetchContent      | BSL-1.0     |

## Build

```bash
# Submodules (necessário apenas uma vez)
git submodule update --init --recursive

# Debug (ASan + UBSan)
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
ctest --test-dir build -V

# Release (SIMD máximo disponível)
cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j$(nproc)
```

## Fases de implementação

| Fase | Módulos               | Status  |
|------|-----------------------|---------|
| 1    | core/ (BitSimulator)  | base    |
| 2    | evolution/ + fitness/ | —       |
| 3    | persistence/          | —       |
| 4    | circuit/Arithmetic    | —       |
| 5    | circuit/SequentialFSM | —       |
| 6    | export/ + equiv check | —       |
| 6A   | BitSimulator SIMD     | —       |
| 6F   | DirtyMask incremental | —       |
| 6G   | EvalSchedule + Cache  | —       |
| 6B   | LUT-k                 | —       |
| 6E   | JIT (LLVM, opcional)  | —       |
| 7    | benchmark/ + CTest    | —       |
