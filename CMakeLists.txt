cmake_minimum_required(VERSION 3.21)

project(chess_ai_genetic LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CHESS_ENABLE_SANITIZERS "Enable address and undefined-behavior sanitizers" OFF)
option(CHESS_ENABLE_PROFILING "Enable gprof instrumentation" OFF)

include(cmake/CompilerWarnings.cmake)
include(cmake/Optimization.cmake)
include(cmake/Sanitizers.cmake)
include(cmake/Profiling.cmake)

add_library(chess_core
    src/chess/piece.cpp
    src/chess/square.cpp
    src/chess/move.cpp
    src/chess/position.cpp
    src/chess/move_generator.cpp
    src/chess/fen.cpp
    src/chess/zobrist.cpp
)
target_include_directories(chess_core PUBLIC src)
chess_set_warnings(chess_core)
chess_enable_sanitizers(chess_core)
chess_enable_profiling(chess_core)

add_library(chess_search
    src/search/evaluation/parameters.cpp
    src/search/evaluation/evaluator.cpp
    src/search/opening_book.cpp
    src/search/move_ordering.cpp
    src/search/search_stats.cpp
    src/search/searcher.cpp
    src/search/chess_ai.cpp
    src/search/transposition_table.cpp
)
target_include_directories(chess_search PUBLIC src)
target_link_libraries(chess_search PUBLIC chess_core)
chess_set_warnings(chess_search)
chess_enable_sanitizers(chess_search)
chess_enable_profiling(chess_search)

add_library(chess_training
    src/training/corpus.cpp
    src/training/individual.cpp
    src/training/population.cpp
    src/training/search_space.cpp
    src/training/mutation.cpp
    src/training/tournament.cpp
    src/training/genetic_trainer.cpp
    src/training/training_config.cpp
)
target_include_directories(chess_training PUBLIC src)
target_link_libraries(chess_training PUBLIC chess_search)
target_compile_definitions(chess_training PRIVATE
    CHESS_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    CHESS_COMPILER_ID="${CMAKE_CXX_COMPILER_ID}"
    CHESS_COMPILER_VERSION="${CMAKE_CXX_COMPILER_VERSION}"
)
chess_set_warnings(chess_training)
chess_enable_sanitizers(chess_training)

add_library(chess_protocol
    src/protocol/game_session.cpp
    src/protocol/protocol.cpp
    src/protocol/command_processor.cpp
)
target_include_directories(chess_protocol PUBLIC src)
target_link_libraries(chess_protocol PUBLIC chess_search)
chess_set_warnings(chess_protocol)
chess_enable_sanitizers(chess_protocol)

add_executable(chess_engine apps/chess_engine/main.cpp)
target_link_libraries(chess_engine PRIVATE chess_protocol)
chess_set_warnings(chess_engine)
chess_enable_sanitizers(chess_engine)

add_executable(train_genetic apps/train/main.cpp)
target_link_libraries(train_genetic PRIVATE chess_training)
chess_set_warnings(train_genetic)
chess_enable_sanitizers(train_genetic)

add_executable(self_play tools/self_play/main.cpp)
target_link_libraries(self_play PRIVATE chess_training)
chess_set_warnings(self_play)
chess_enable_sanitizers(self_play)

add_executable(fen_eval tools/fen_eval/main.cpp)
target_link_libraries(fen_eval PRIVATE chess_search)
chess_set_warnings(fen_eval)
chess_enable_sanitizers(fen_eval)

add_executable(build_opening_book tools/opening_book_builder/main.cpp)
target_link_libraries(build_opening_book PRIVATE chess_core)
chess_set_warnings(build_opening_book)
chess_enable_sanitizers(build_opening_book)

add_executable(compare_models tools/model_compare/main.cpp)
target_link_libraries(compare_models PRIVATE chess_training)
chess_set_warnings(compare_models)
chess_enable_sanitizers(compare_models)

add_executable(search_benchmark benchmarks/search_benchmark.cpp)
target_link_libraries(search_benchmark PRIVATE chess_search)
target_compile_definitions(search_benchmark PRIVATE
    CHESS_ARCHITECTURE="${CMAKE_SYSTEM_PROCESSOR}"
    CHESS_COMPILER_ID="${CMAKE_CXX_COMPILER_ID}"
    CHESS_COMPILER_VERSION="${CMAKE_CXX_COMPILER_VERSION}"
    CHESS_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    CHESS_OPTIMIZATION="compiler-default+CMake-${CMAKE_BUILD_TYPE}"
)
chess_set_warnings(search_benchmark)
chess_enable_sanitizers(search_benchmark)
chess_enable_profiling(search_benchmark)

enable_testing()

add_executable(chess_tests
    tests/test_main.cpp
    tests/unit/chess/test_position.cpp
    tests/unit/chess/test_moves.cpp
    tests/unit/chess/test_rules.cpp
    tests/unit/chess/test_fen.cpp
    tests/unit/search/test_ai.cpp
    tests/unit/search/test_opening_book.cpp
    tests/unit/training/test_genetic.cpp
    tests/perft/perft_tests.cpp
    tests/integration/test_protocol.cpp
)
target_link_libraries(chess_tests PRIVATE chess_protocol chess_training)
target_include_directories(chess_tests PRIVATE tests/support)
target_compile_definitions(chess_tests PRIVATE CHESS_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
chess_set_warnings(chess_tests)
chess_enable_sanitizers(chess_tests)

add_test(NAME chess_tests COMMAND chess_tests)

chess_enable_release_ipo()
