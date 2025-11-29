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
  src/renderer.cpp
  src/input.cpp
  src/ncurses_terminal.cpp
  src/file_reader.cpp
  src/editor.cpp
  src/undo_manager.cpp
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
