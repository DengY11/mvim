cmake_minimum_required(VERSION 3.16)
project(mvim LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(mvim
  src/main.cpp
  src/terminal.cpp
  src/text_buffer.cpp
  src/gap_buffer.cpp
  src/line_index.cpp
  src/gap_text_buffer_core.cpp
  src/rope_text_buffer_core.cpp
  src/renderer.cpp
  src/input.cpp
  src/ncurses_terminal.cpp
  src/file_reader.cpp
  src/editor_commands.cpp
  src/editor.cpp
  src/undo_manager.cpp
  src/pane_layout.cpp
)
target_compile_features(mvim PRIVATE cxx_std_20)

target_link_libraries(mvim PRIVATE ${CURSES_LIBRARIES})

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
if (ENABLE_ASAN)
  target_compile_options(mvim PRIVATE -fsanitize=address -fno-omit-frame-pointer)
  target_link_options(mvim PRIVATE -fsanitize=address)
endif()

target_compile_options(mvim PRIVATE -Wall -Wextra -Wpedantic)

message(STATUS "CXX standard: ${CMAKE_CXX_STANDARD}")

add_executable(mvim_tests
  src/text_buffer.cpp
  src/gap_buffer.cpp
  src/line_index.cpp
  src/gap_text_buffer_core.cpp
  src/rope_text_buffer_core.cpp
  src/file_reader.cpp
  src/pane_layout.cpp
  tests/test_text_buffer.cpp
  tests/test_layout.cpp
)
target_compile_features(mvim_tests PRIVATE cxx_std_20)
target_compile_options(mvim_tests PRIVATE -Wall -Wextra -Wpedantic)
target_include_directories(mvim_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_executable(mvim_backends_bench
  src/text_buffer.cpp
  src/gap_text_buffer_core.cpp
  # piece table removed
  src/rope_text_buffer_core.cpp
  src/vector_text_buffer_core.hpp
  src/gap_buffer.cpp
  src/line_index.cpp
  src/file_reader.cpp
  tests/bench_backends.cpp
)
target_compile_features(mvim_backends_bench PRIVATE cxx_std_20)
target_compile_options(mvim_backends_bench PRIVATE -O2)
target_include_directories(mvim_backends_bench PRIVATE ${CMAKE_SOURCE_DIR}/src)
