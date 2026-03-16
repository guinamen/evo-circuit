cmake_minimum_required(VERSION 3.20)
project(evo-circuit
    VERSION 0.1.0
    DESCRIPTION "Evolutionary digital circuit synthesis via CGP (C++17)"
    LANGUAGES CXX
)

# ── Padrão C++17 ───────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Opções de build ────────────────────────────────────────────────────────
option(EVO_CIRCUIT_JIT      "Habilitar módulo JIT via LLVM OrcJIT"        OFF)
option(EVO_CIRCUIT_TESTS    "Habilitar suite de testes CTest"              ON)
option(EVO_CIRCUIT_WARNINGS "Habilitar flags de warning extras"            ON)

# ── Flags por tipo de build ────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Tipo de build" FORCE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -march=native -DNDEBUG)
    message(STATUS "Build Release: -O3 -march=native (SIMD máximo disponível)")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
    # ASan + UBSan em Debug — desabilita SIMD para legibilidade
    add_compile_options(-fsanitize=address,undefined)
    add_link_options(-fsanitize=address,undefined)
    message(STATUS "Build Debug: ASan + UBSan habilitados")
endif()

if(EVO_CIRCUIT_WARNINGS)
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wno-unused-parameter)
endif()

# spdlog: habilitar TRACE em Debug, DEBUG em Release
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
else()
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

# ── Dependências externas ──────────────────────────────────────────────────

# cgp-plusplus — sem CMakeLists.txt próprio; expõe headers via INTERFACE
if(EXISTS "${CMAKE_SOURCE_DIR}/extern/cgp-plusplus")
    add_library(cgp-plusplus INTERFACE)

# cgp-plusplus possui dois .cpp que precisam de compilação
add_library(cgp-plusplus-impl STATIC
    extern/cgp-plusplus/parameters/Parameters.cpp
    extern/cgp-plusplus/random/Random.cpp
)
target_include_directories(cgp-plusplus-impl PUBLIC extern/cgp-plusplus)
target_compile_features(cgp-plusplus-impl PUBLIC cxx_std_17)
target_compile_options(cgp-plusplus-impl PRIVATE
    -Wno-shadow -Wno-vla -Wno-pedantic
)
    target_include_directories(cgp-plusplus INTERFACE
        extern/cgp-plusplus
    )
    message(STATUS "cgp-plusplus: headers em extern/cgp-plusplus")
else()
    message(WARNING "cgp-plusplus não encontrado em extern/cgp-plusplus — "
                    "execute: git submodule update --init extern/cgp-plusplus")
    add_library(cgp-plusplus INTERFACE)

# cgp-plusplus possui dois .cpp que precisam de compilação
add_library(cgp-plusplus-impl STATIC
    extern/cgp-plusplus/parameters/Parameters.cpp
    extern/cgp-plusplus/random/Random.cpp
)
target_include_directories(cgp-plusplus-impl PUBLIC extern/cgp-plusplus)
target_compile_features(cgp-plusplus-impl PUBLIC cxx_std_17)
target_compile_options(cgp-plusplus-impl PRIVATE
    -Wno-shadow -Wno-vla -Wno-pedantic
)
    target_include_directories(cgp-plusplus INTERFACE extern/cgp-plusplus)
endif()

# SQLiteCpp
if(EXISTS "${CMAKE_SOURCE_DIR}/extern/SQLiteCpp/CMakeLists.txt")
    set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE)
    set(SQLITECPP_RUN_CPPLINT  OFF CACHE BOOL "" FORCE)
    set(SQLITECPP_BUILD_TESTS  OFF CACHE BOOL "" FORCE)
    add_subdirectory(extern/SQLiteCpp EXCLUDE_FROM_ALL)
    message(STATUS "SQLiteCpp: submodule encontrado")
else()
    message(WARNING "SQLiteCpp não encontrado — "
                    "execute: git submodule update --init extern/SQLiteCpp")
endif()

# xsimd (header-only)
if(EXISTS "${CMAKE_SOURCE_DIR}/extern/xsimd/include")
    add_library(xsimd INTERFACE)
    target_include_directories(xsimd INTERFACE extern/xsimd/include)
    message(STATUS "xsimd: header-only em extern/xsimd/include")
else()
    message(WARNING "xsimd não encontrado — "
                    "execute: git submodule update --init extern/xsimd")
    add_library(xsimd INTERFACE)
    target_include_directories(xsimd INTERFACE extern/xsimd/include)
endif()

# OpenMP
find_package(OpenMP REQUIRED)

# spdlog — logging estruturado
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)

# LLVM OrcJIT (opcional)
if(EVO_CIRCUIT_JIT)
    find_package(LLVM 17 REQUIRED CONFIG)
    message(STATUS "LLVM ${LLVM_PACKAGE_VERSION} encontrado: ${LLVM_DIR}")
    include_directories(${LLVM_INCLUDE_DIRS})
    add_definitions(${LLVM_DEFINITIONS})
endif()

# ── Biblioteca core ────────────────────────────────────────────────────────
add_library(evo-circuit-core
    src/core/BitSimulator.cpp
    src/core/DirtyMask.cpp
    src/core/PhenotypeCache.cpp
)

target_include_directories(evo-circuit-core
    PUBLIC  include
    PRIVATE include/evo-circuit/core
)

target_link_libraries(evo-circuit-core
    PUBLIC cgp-plusplus cgp-plusplus-impl xsimd OpenMP::OpenMP_CXX
    PUBLIC spdlog::spdlog fmt::fmt
)

# ── Executável principal (placeholder) ────────────────────────────────────
add_executable(evo-circuit src/main.cpp)
target_link_libraries(evo-circuit PRIVATE evo-circuit-core)

if(TARGET SQLiteCpp)
    target_link_libraries(evo-circuit PRIVATE SQLiteCpp)
endif()

# ── Testes ─────────────────────────────────────────────────────────────────
if(EVO_CIRCUIT_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# ── Sumário de configuração ────────────────────────────────────────────────
message(STATUS "")
message(STATUS "══ evo-circuit configuração ══════════════════")
message(STATUS "  Build type   : ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard : ${CMAKE_CXX_STANDARD}")
message(STATUS "  JIT (LLVM)   : ${EVO_CIRCUIT_JIT}")
message(STATUS "  Testes       : ${EVO_CIRCUIT_TESTS}")
message(STATUS "═══════════════════════════════════════════════")

# ── Biblioteca evolution (Fase 2) ──────────────────────────────────────────
add_library(evo-circuit-evolution
    src/evolution/BooleanEvolver.cpp
)

target_include_directories(evo-circuit-evolution
    PUBLIC  include
    # Expõe todos os headers do cgp-plusplus para quem usar esta lib
    PUBLIC  extern/cgp-plusplus
)

target_link_libraries(evo-circuit-evolution
    PUBLIC evo-circuit-core
    PUBLIC cgp-plusplus-impl
)

# ── Timing benchmark (profiling manual) ────────────────────────────────────
add_executable(timing_benchmark profiling/timing_benchmark.cpp)
target_include_directories(timing_benchmark PRIVATE
    include
    extern/cgp-plusplus
)
target_link_libraries(timing_benchmark PRIVATE
    evo-circuit-evolution
    spdlog::spdlog fmt::fmt
)
target_compile_options(timing_benchmark PRIVATE
    -Wno-shadow -Wno-vla -Wno-pedantic
)
