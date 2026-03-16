# Tenta usar Catch2 instalado no sistema; se não encontrar, baixa via FetchContent
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

add_executable(test_core test_core.cpp)
target_link_libraries(test_core PRIVATE evo-circuit-core Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(test_core)
