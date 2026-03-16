# ── Catch2 ─────────────────────────────────────────────────────────────────
find_package(Catch2 3 QUIET)

if(NOT Catch2_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.3
    )
    FetchContent_MakeAvailable(Catch2)
    message(STATUS "Catch2: baixado via FetchContent")
else()
    message(STATUS "Catch2: encontrado no sistema")
endif()

include(CTest)
include(Catch)

# ── test_core (Fase 1) ──────────────────────────────────────────────────────
add_executable(test_core test_core.cpp)
target_link_libraries(test_core PRIVATE
    evo-circuit-core
    Catch2::Catch2WithMain
)
target_compile_definitions(test_core PRIVATE CATCH_CONFIG_FAST_COMPILE)
catch_discover_tests(test_core
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# ── test_phase2 (Fase 2) ────────────────────────────────────────────────────
add_executable(test_phase2 test_phase2.cpp)

target_include_directories(test_phase2 PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/extern/cgp-plusplus"
)

target_link_libraries(test_phase2 PRIVATE
    evo-circuit-evolution
    Catch2::Catch2WithMain
)

# Suprimir warnings dos headers de terceiros (cgp-plusplus)
target_compile_options(test_phase2 PRIVATE
    -Wno-shadow
    -Wno-vla
    -Wno-pedantic
)

target_compile_definitions(test_phase2 PRIVATE CATCH_CONFIG_FAST_COMPILE)

# WORKING_DIRECTORY = raiz do repositório para que find_plu() encontre
# data/plufiles/epar3.plu com caminho relativo "data/plufiles/epar3.plu"
catch_discover_tests(test_phase2
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
