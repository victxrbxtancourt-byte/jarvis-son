cmake_minimum_required(VERSION 3.16)
project(jarvis_son LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(JARVIS_BUILD_TESTS "Compilar y registrar las pruebas" ON)

# Biblioteca con la lógica del orquestador (solo POSIX: compila igual en
# Termux/Android y en Linux, por ejemplo en GitHub Actions).
add_library(jarvis_core
  src/config.cpp
  src/orchestrator.cpp
  src/process.cpp
  src/state.cpp
  src/text.cpp)
target_include_directories(jarvis_core PUBLIC include)
target_compile_options(jarvis_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(jarvis-son src/main.cpp)
target_link_libraries(jarvis-son PRIVATE jarvis_core)
target_compile_options(jarvis-son PRIVATE -Wall -Wextra -Wpedantic)

if(JARVIS_BUILD_TESTS)
  enable_testing()

  add_executable(test_text tests/test_text.cpp)
  target_link_libraries(test_text PRIVATE jarvis_core)
  target_include_directories(test_text PRIVATE tests)
  add_test(NAME texto COMMAND test_text)

  add_executable(test_process tests/test_process.cpp)
  target_link_libraries(test_process PRIVATE jarvis_core)
  target_include_directories(test_process PRIVATE tests)
  add_test(NAME procesos COMMAND test_process)

  add_test(NAME turno_con_stubs
           COMMAND sh ${CMAKE_SOURCE_DIR}/tests/run_stub_turn.sh $<TARGET_FILE:jarvis-son>)
endif()
